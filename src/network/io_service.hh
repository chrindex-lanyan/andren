#pragma once 

#include "../base/andren_base.hh"
#include "events_service.hh"
#include <deque>
#include <functional>
#include <future>
#include <liburing.h>
#include <memory>
#include <sys/socket.h>



namespace chrindex::andren::network
{
    /// io request structure
    /// sizeof 1024 
    struct io_request
    {
        enum REQ { OPEN, CONNECT, ACCEPT, 
            READ, WRITE, DISCONNECT, CLOSE, HOSTING, OTHER };

        struct general_t
        {
            REQ _IN_OUT_ req;
            int64_t uid;
            uint32_t _IN_OUT_ size; // socklen / path len / buf len 
            int32_t _IN_ flags; // for : file / socket
            int32_t _IN_OUT_ fd;// for : file / socket
        } general;

        union {
            struct addr_t{
                sockaddr_storage _OUT_ ss;
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

#define _BUF_SIZE_ (1024 - ( sizeof(general) + sizeof(_others_structure)))
                // for : read , write
                char _IN_OUT_ buf[_BUF_SIZE_]; 
            } bufData;
        } _IN_OUT_ ioData ;
    };

    struct io_context
    {
        io_context() : req_real(sizeof(io_request),0)
        {
            req_context = reinterpret_cast<io_request*>(&req_real[0]);
        }
        DEFAULT_DECONSTRUCTION(io_context);
        
        io_context(io_context const & other) 
        {
            onEvents = other.onEvents;
            userdata = other.userdata;
            req_real = other.req_real;
            req_context = reinterpret_cast<io_request*>(&req_real[0]);
        }
        
        void operator=(io_context const & other)
        {
            onEvents = other.onEvents;
            userdata = other.userdata;
            req_real = other.req_real;
            req_context = reinterpret_cast<io_request*>(&req_real[0]);
        }
        
        DEFAULT_MOVE_CONSTRUCTION(io_context);
        DEFAULT_MOVE_OPERATOR(io_context);
        
        std::function<void(io_context *)> onEvents;
        std::any userdata;
        std::string req_real;
        io_request* req_context;
    };

    class IOService : public EventsService ,base::noncopyable
    {
    public :

        IOService (int64_t key);
        IOService (IOService && ios) noexcept;
        ~IOService();

        void operator= (IOService && ios) noexcept;

        bool submitRequest(int uid, io_context && context);

    private :

        io_uring * init_a_new_io_uring(int used);

        io_uring_sqe * get_sqe_and_record_one();

        io_uring_sqe * find_empty_sqe(io_uring ** ppuring);

        void deinit_io_uring();

        void init();

    private :
        std::map<int , io_context> m_fds_context;
        base::MinHeap<base::FourWayHeap<int, std::shared_ptr<io_uring>>> m_uring;
    };

}