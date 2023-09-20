#include "io_service.hh"
#include "events_service.hh"


#include <chrono>
#include <liburing.h>
#include <memory>
#include <sys/socket.h>
#include <utility>


namespace chrindex::andren::network
{
    static inline constexpr auto DEFAULT_QUEUE_SIZE = 128;

    IOService::IOService (int64_t key) : EventsService(key)
    {
        init_a_new_io_uring(0);
    }
    IOService::IOService (IOService && ios) noexcept : EventsService(std::move(ios))
    {
        m_fds_context = std::move(ios.m_fds_context);
        m_uring = std::move(ios.m_uring);
    }

    IOService::~IOService()
    {
        deinit_io_uring();
    }

    void IOService::operator= (IOService && ios) noexcept
    {
        this->~IOService();
        EventsService::operator=(std::move(ios));
        m_fds_context = std::move(ios.m_fds_context);
        m_uring = std::move(ios.m_uring);
    }

    bool IOService::submitRequest(uint64_t uid, io_context && _context)
    {
        auto & context = m_fds_context[uid];

        context = std::move(_context);

        if ( context.req_context->general.req == io_request::OTHER 
        || context.req_context->general.req == io_request::HOSTING 
        || context.req_context->general.req == io_request::DISCONNECT)
        {
            return false;
        }

        io_uring * puring = nullptr;
        auto sqe = find_empty_sqe(&puring);
        sqe->user_data = reinterpret_cast<uint64_t>(&context);

        switch (context.req_context->general.req) 
        {
        case io_request::OPEN:
        {
            // fd 在 CQE 的 res 字段返回
            io_uring_prep_openat(sqe, 
                context.req_context->ioData.file.dfd, 
                context.req_context->ioData.file.path, 
                context.req_context->general.flags, 
                context.req_context->ioData.file.mode);
            break;
        }
        case io_request::CONNECT:
        {
            // ret 在 CQE 的 res 字段
            io_uring_prep_connect(sqe, 
                context.req_context->general.fd, 
                reinterpret_cast<sockaddr*>(&context.req_context->ioData.addr.ss),
                context.req_context->general.size);
            break;
        }
        case io_request::ACCEPT:
        {
            io_uring_prep_accept(sqe, 
                context.req_context->general.fd, 
                reinterpret_cast<sockaddr*>(&context.req_context->ioData.addr.ss),
                &context.req_context->general.size ,
                context.req_context->general.flags);
            break;
        }
        case io_request::READ:
        {
            io_uring_prep_recv(sqe, 
                context.req_context->general.fd, 
                context.req_context->ioData.bufData.buf , 
                sizeof(context.req_context->ioData.bufData.buf),
                context.req_context->general.flags);
            break;
        }
        case io_request::WRITE:
        {
            io_uring_prep_send(sqe, 
                context.req_context->general.fd, 
                context.req_context->ioData.bufData.buf_ptr, 
                context.req_context->general.size, 
                context.req_context->general.flags);
            break;
        }
        case io_request::CLOSE:
        {
            io_uring_prep_close(sqe, context.req_context->general.fd);
            break;
        }
        default: // ignore
        {
            return false;
        }
        }
        return true;
    }

    io_uring_sqe * IOService::find_empty_sqe(io_uring ** ppuring)
    {
        io_uring_sqe * ptr = nullptr;
        int count = m_uring.size();

        for (int i =0 ; i < count ;i++)
        {
            if(auto result = m_uring.min_pair() ;
               result.has_value() && result->key() < DEFAULT_QUEUE_SIZE)
            {
                if (auto p = io_uring_get_sqe(result->value().get()) 
                    ; p != nullptr)
                {
                    ptr = p;
                    *ppuring = result->value().get();
                    result = m_uring.extract_min_pair();
                    m_uring.push( {result->key() + 1 , result->value()});
                    break;
                }
            }
        }

        if (ptr == nullptr)
        {
            auto puring = init_a_new_io_uring(1);
            ptr = io_uring_get_sqe(puring);
        }

        return ptr;
    }

    io_uring * IOService::init_a_new_io_uring(int used)
    {
        auto puring = std::make_shared<io_uring>();
        m_uring.push( {used, puring });
        io_uring_queue_init(DEFAULT_QUEUE_SIZE, puring.get(), 0);
        return puring.get();
    }

    void IOService::deinit_io_uring()
    {
        m_uring.foreach_pair([](auto & pair)
        {
            if (pair.value())
            {
                auto result = pair.value().get();
                io_uring_queue_exit(result);
            }
        });
    }

    void IOService::init()
    {
        setNotifier([this](std::vector<base::KVPair<uint64_t, int>> & events [[maybe_unused]])
        {
            auto tmp_uring = std::move(m_uring);
            tmp_uring.foreach_pair([&events, this](base::KVPair<int, std::shared_ptr<io_uring>> & pair)
            {
                auto submit_count = pair.key();
                auto puring = pair.value().get();
                int ret = io_uring_submit(puring);
                if (ret < 0)
                {
                    /// error
                }
                struct io_uring_cqe * pcqe = 0;
                struct __kernel_timespec kspec;

                kspec.tv_nsec =  std::min(100 , submit_count * 1);
                kspec.tv_sec = 0;

                ret = io_uring_wait_cqes(puring, &pcqe, submit_count, &kspec, 0);

                if (ret !=0)
                {
                    /// error
                }
                uint32_t head = 0;
                int count =0;
                io_uring_for_each_cqe(puring,head,pcqe)
                {
                    uint64_t ioctx_ptr = pcqe->user_data;
                    events.push_back({ ioctx_ptr, pcqe->res});
                    count++;
                }
                io_uring_cq_advance(puring, count);

                /// 更新剩余提交数后放回uring结构
                m_uring.push({submit_count - count , std::move(pair.value())});
            });
        });

        setEventsHandler([this](uint64_t key [[maybe_unused]] , int cqe_res [[maybe_unused]]) 
        {
            auto ioctx_ptr = reinterpret_cast<io_context *>(key);
            auto iter = m_fds_context.find(ioctx_ptr->req_context->general.uid);
            bool bret = true;
            
            if(ioctx_ptr->onEvents)
            {
                bret = ioctx_ptr->onEvents(ioctx_ptr, cqe_res);
            }
            if (bret)
            {
                m_fds_context.erase(iter);  
            }
        });
    }

}

