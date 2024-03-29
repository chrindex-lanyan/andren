﻿#pragma once 

#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <liburing.h>
#include <memory>
#include <sys/socket.h>
#include <any>
#include <vector>
#include <map>

#include "base_types.hh"
#include "noncopyable.hpp"
#include "mutex_que.hpp"

#include "events_service.hh"

namespace chrindex::andren::network
{
    static inline constexpr auto DEFAULT_QUEUE_SIZE = 128;

    /// io request structure
    /// size 1024 
    struct io_request
    {
        enum REQ { OPEN, CONNECT, ACCEPT, 
            READ, WRITE , RECV, SEND, DISCONNECT, CLOSE, HOSTING, OTHER };

        struct general_t
        {
            REQ _IN_OUT_ req;
            uint64_t _IN_ uid;
            uint32_t _IN_OUT_ size; // socklen / path len / buf len 
            int32_t _IN_ flags; // for : file / socket
            int32_t _IN_OUT_ fd;// for : file / socket
        } general;

        union {
            struct addr_t{
                sockaddr_storage _IN_OUT_ ss;
            } addr;// for : accept

            struct file_t{
                int32_t _IN_ dfd;
                uint32_t _IN_ mode;
                char _IN_ path[256]; 
            } file;// for : open

            struct buf_data_t
            {
#define _OTHER_STRUCTURE_SIZE_ (std::max(sizeof(file_t), sizeof(addr_t)))
                // for other structure
                char _IN_OUT_ _others_structure[_OTHER_STRUCTURE_SIZE_];

                char * _IN_ buf_ptr; // for : write

#define _BUF_SIZE_ (1024 - ( sizeof(general) + sizeof(_others_structure)))
                // for : read 
                char _IN_OUT_ buf[_BUF_SIZE_]; 
            } bufData;
        } _IN_OUT_ ioData ;
    };

    struct io_context :public base::noncopyable
    {
        io_context() : req_context(std::make_unique<io_request>())
        {
            //
        }
        DEFAULT_DECONSTRUCTION(io_context);
        
        io_context(io_context && other) 
        {
            onEvents = std::move(other.onEvents);
            userdata = std::move(other.userdata);
            req_context = std::move(other.req_context);
        }
        
        void operator=(io_context && other)
        {
            onEvents = std::move(other.onEvents);
            userdata = std::move(other.userdata);
            req_context = std::move(other.req_context);
        }

        
        /// @brief events done.
        /// @param io_context* last submited io_context
        /// @return Return the `true` indicates that the io operation 
        /// submitted to io_service has been completed, and then the 
        /// events should be removed by io_service; 
        /// otherwise `false` should be returned.
        std::function<bool (io_context *, int32_t cqe_res)> onEvents;
        //std::shared_ptr<void> userdata;
        std::any userdata;
        std::unique_ptr<io_request> req_context;
    };

    class IOService : public EventsService ,base::noncopyable
    {
    public :

        enum class DriverMode:int32_t
        {
            DEFAULT_IRQ,  // Interrupt ReQuest
            SETUP_SQPOLL, // polling used kernel-thread 
            SETUP_IOPOLL, // Need DIO and more...
        };

        IOService (int64_t key, uint32_t entries_size = DEFAULT_QUEUE_SIZE, DriverMode mode = DriverMode::DEFAULT_IRQ);
        IOService(int64_t key, uint32_t entries_size ,std::unique_ptr<io_uring> && another);
        IOService (IOService && ios) noexcept;
        ~IOService();

        void operator= (IOService && ios) noexcept;

        void init(sigset_t sigmask);

        bool submitRequest(uint64_t uid, io_context && context);

        uint32_t sqe_used() const;

        uint32_t entries_size() const;

    private :

        io_uring * init_a_new_io_uring(uint32_t size, DriverMode mode,std::unique_ptr<io_uring> && another);

        io_uring_sqe * find_empty_sqe();

        void deinit_io_uring();

        void working_request();

        bool submitRequest_Private(uint64_t uid, std::shared_ptr<io_context> sp_context);

    private :
        std::map<uint64_t , io_context> m_fds_context;
        std::unique_ptr<base::mutex_que<std::function<void()>>> m_request_submit_queue;
        uint32_t m_size;
        std::atomic<uint32_t> m_used;
        std::unique_ptr<io_uring> m_uring;
    };

}