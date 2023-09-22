#include "io_service.hh"
#include "events_service.hh"


#include <asm-generic/errno.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <memory>
#include <sys/socket.h>
#include <utility>
#include <sys/types.h>


namespace chrindex::andren::network
{
    

    IOService::IOService (int64_t key, uint32_t entries_size, DriverMode mode) : EventsService(key)
    {
        init_a_new_io_uring(entries_size,mode,nullptr);
    }

    IOService::IOService(int64_t key, uint32_t entries_size , std::unique_ptr<io_uring> && another): EventsService(key)
    {
        init_a_new_io_uring(entries_size,DriverMode::DEFAULT_IRQ,std::move(another));
    }

    IOService::IOService (IOService && ios) noexcept : EventsService(std::move(ios))
    {
        m_fds_context = std::move(ios.m_fds_context);
        m_uring = std::move(ios.m_uring);
        m_request_submit_queue = std::move(ios.m_request_submit_queue);
        m_size = ios.m_size;
        m_used = ios.m_used.load(std::memory_order_seq_cst);
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

    bool IOService::submitRequest (uint64_t uid, io_context && context)
    {
        if (!m_uring || !m_request_submit_queue)
        {
            return false;
        }
        std::shared_ptr<io_context> sp_context = std::make_shared<io_context>(std::move(context));
        m_request_submit_queue->pushBack([this, uid, sp_context]() mutable
        {
            submitRequest_Private(uid, std::move(sp_context));
        });
        return true;
    }

    bool IOService::submitRequest_Private(uint64_t uid, std::shared_ptr<io_context> sp_context)
    {
        auto & context = m_fds_context[uid];

        context = std::move(*sp_context);

        if ( context.req_context->general.req == io_request::OTHER 
        || context.req_context->general.req == io_request::HOSTING 
        || context.req_context->general.req == io_request::DISCONNECT)
        {
            return false;
        }

        auto sqe = find_empty_sqe();
        if (sqe == nullptr)
        {
            return false;
        }
        
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
        io_uring_sqe_set_data(sqe, &context);
        m_used.fetch_add(1,std::memory_order_seq_cst);
        return true;
    }

    io_uring_sqe * IOService::find_empty_sqe()
    {
        if (m_uring == nullptr)
        {
            return nullptr;
        }
        return io_uring_get_sqe(m_uring.get());
    }

    io_uring * IOService::init_a_new_io_uring(uint32_t size,DriverMode _mode,std::unique_ptr<io_uring> && another)
    {
        m_request_submit_queue = std::make_unique<base::DBuffer<std::function<void()>>>();
        if (another)
        {
            m_uring = std::move(another);
        }
        else 
        {
            uint32_t mode =0;

            if (_mode == DriverMode::DEFAULT_IRQ)
            {
    ;           mode = 0;
            }
            else if(_mode == DriverMode::SETUP_SQPOLL)
            { 
                mode = IORING_SETUP_SQPOLL;
            }
            else if(_mode == DriverMode::SETUP_IOPOLL)
            {
                mode = IORING_SETUP_IOPOLL;
            }
            m_uring = std::make_unique<io_uring>();
            io_uring_queue_init(size, m_uring.get(), mode);
        }
        m_size = size;
        m_used.store(0,std::memory_order_seq_cst);
        return m_uring.get();
    }

    void IOService::deinit_io_uring()
    {
        if (m_uring != nullptr)
        {
            io_uring_queue_exit(m_uring.get());
            m_uring.reset(nullptr);
        }
    }

    void IOService::init(sigset_t sigmask)
    {
        setNotifier([this,sigmask](std::vector<base::KVPair<uint64_t, int>> & events)mutable
        {
            working_request();

            uint32_t submit_count = sqe_used();
            auto puring = m_uring.get();
            int ret = 0;

            if (submit_count <=0)
            {
                /// no request need submit.
                return ;
            }

            ret = io_uring_submit(puring);
            if (ret < 0)
            {
                /// error
                printf ("IOService::init:: Submit Failed."
                    " Submit Count=%d. ret = %d.\n",submit_count,ret);
                return ;
            }
            struct io_uring_cqe * pcqe = 0;
            struct __kernel_timespec kspec;
            
            kspec.tv_nsec = 1000;
            kspec.tv_sec = 0;
            
            ret = io_uring_wait_cqes(puring, &pcqe, 
                1, &kspec, &sigmask);// 阻塞等待
            if (ret !=0 && errno != ETIME)
            {
                printf ("IOService::init:: Wait CQE Failed. errno = [%d : %m].\n", errno, errno);
                return ;
            }
            
            int count =0;
            uint32_t head = 0;
            io_uring_for_each_cqe(puring,head,pcqe)
            {
                uint64_t ioctx_ptr = reinterpret_cast<uint64_t>(io_uring_cqe_get_data(pcqe));
                if(ioctx_ptr && ioctx_ptr !=(~0UL))
                {
                    events.push_back({ ioctx_ptr, pcqe->res});    
                }
                else 
                {
                    printf("IOService::init:: Failed To Deal"
                        " With Event Due to Context Address = %lu.\n",ioctx_ptr);
                }
                count++;
            }

            /// 更新剩余
            if(count > 0)
            {
                io_uring_cq_advance(puring, count);
                m_used.fetch_sub(count,std::memory_order_seq_cst);
            }
        });

        setEventsHandler([this](uint64_t key, int cqe_res) 
        {
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

    uint32_t IOService::sqe_used() const
    {
        return m_size;
    }

    uint32_t IOService::entries_size() const
    {
        return m_used.load(std::memory_order_seq_cst);
    }

    void IOService::working_request()
    {
        if (!m_request_submit_queue)
        {
            return ;
        }
        std::deque<std::function<void ()>> results;
        m_request_submit_queue->takeMulti(results);
        for (auto & fn : results )
        {
            fn();
        }
    }

}

