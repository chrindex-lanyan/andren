#include "io_service.hh"
#include "events_service.hh"


#include <chrono>
#include <cstdio>
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
        io_uring_sqe_set_data(sqe, &context);
        printf("IOService::submitRequest:: Context Address = %llu.\n",sqe->user_data);
        printf("IOService::submitRequest:: uring address = %lu.\n", (uint64_t)puring);

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
        case io_request::RECV:
        {
            io_uring_prep_recv(sqe, 
                context.req_context->general.fd, 
                context.req_context->ioData.bufData.buf , 
                sizeof(context.req_context->ioData.bufData.buf),
                context.req_context->general.flags);
            break;
        }
        case io_request::SEND:
        {
            io_uring_prep_send(sqe, 
                context.req_context->general.fd, 
                context.req_context->ioData.bufData.buf_ptr, 
                context.req_context->general.size, 
                context.req_context->general.flags);
            break;
        }
        case io_request::READ:
        {
            io_uring_prep_read(sqe, 
                context.req_context->general.fd, 
                context.req_context->ioData.bufData.buf , 
                sizeof(context.req_context->ioData.bufData.buf),
                0);
            break;
        }
        case io_request::WRITE:
        {
            io_uring_prep_write(sqe, 
                context.req_context->general.fd, 
                context.req_context->ioData.bufData.buf_ptr, 
                context.req_context->general.size, 
                0);
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
                    auto result = m_uring.extract_min_pair();
                    m_uring.push( {result->key() + 1 , result->value()});
                    break;
                }
            }
        }

        if (ptr == nullptr)
        {
            auto puring = init_a_new_io_uring(1);
            ptr = io_uring_get_sqe(puring);
            *ppuring = puring;
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
        setNotifier([this](std::vector<base::KVPair<uint64_t, int>> & events)
        {
            //printf ("IOService::init:: Notifier .\n");
            auto tmp_uring = std::move(m_uring);
            tmp_uring.foreach_pair([&events, this](base::KVPair<int, std::shared_ptr<io_uring>> & pair)
            {
                auto submit_count = pair.key();
                auto puring = pair.value().get();

                if (submit_count <=0)
                {
                    /// no request need submit.
                    return ;
                }

                int ret = io_uring_submit(puring);
                if (ret < 0)
                {
                    /// error
                    printf ("IOService::init:: submit failed."
                        " submit count=%d. ret = %d.\n",submit_count,ret);
                    return ;
                }
                printf ("IOService::init:: submit count = %d .\n",submit_count);
                struct io_uring_cqe * pcqe = 0;
                struct __kernel_timespec kspec;

                kspec.tv_nsec =  std::min(100 , submit_count * 1);
                kspec.tv_sec = 0;

                ret = io_uring_wait_cqes(puring, &pcqe, submit_count, &kspec, 0);

                if (ret !=0)
                {
                    printf ("IOService::init:: wait cqe failed. errno str = %m.\n",errno);
                    return ;
                }
                uint32_t head = 0;
                int count =0;
                io_uring_for_each_cqe(puring,head,pcqe)
                {
                    uint64_t ioctx_ptr = reinterpret_cast<uint64_t>(io_uring_cqe_get_data(pcqe));
                    events.push_back({ ioctx_ptr, pcqe->res});
                    printf("IOService::init:: Push Address = %lu.\n",ioctx_ptr);
                    count++;
                }
                io_uring_cq_advance(puring, count);
                printf("IOService::init:: Found %d Request Finished. "
                    " uring address = %lu.\n",count,(uint64_t)puring);

                /// 更新剩余提交数后放回uring结构
                m_uring.push({submit_count - count , std::move(pair.value())});
            });
        });

        setEventsHandler([this](uint64_t key, int cqe_res) 
        {
            printf("IOService::init:: Context Address = %lu.\n",key);
            auto ioctx_ptr = reinterpret_cast<io_context *>(key);
            if (ioctx_ptr == nullptr)
            {
                /// error
                printf("IOService::init:: cannot get user data from CQE::user_data.\n");
                return ;
            }

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

