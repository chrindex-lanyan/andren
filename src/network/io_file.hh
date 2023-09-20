#pragma once

#include "../base/andren_base.hh"
#include "io_service.hh"
#include <any>
#include <functional>
#include <memory>
#include <sys/socket.h>
#include <utility>


namespace chrindex::andren::network
{
    class io_file
    {
    public :

        io_file();

        io_file(std::string const & path, uint32_t flags , uint32_t mode = -1);

        io_file(io_file && another);

        ~io_file();

        void async_open(std::function<void(io_file *self, int32_t ret)> onOpen , IOService & ioservice  ,
            std::string const & path, int dir_fd, uint32_t flags , uint32_t mode = -1);

        void async_write(std::function<void (io_file *self, ssize_t wrsize)> onWrite , 
            IOService & ioservice ,std::string data , int flags);

        void async_read(std::function<void(io_file *self, std::string && data , size_t rdsize )> onRead ,
            IOService & ioservice , int flags);

        void async_close(std::function<void (io_file * self)> onClose , 
            IOService & ioservice );

        int immediately_open(std::string const & path, uint32_t flags , uint32_t mode =-1);

        ssize_t immediately_write(char const * data , size_t size);

        ssize_t immediately_read(std::string & data);

        void immediately_close();

        bool valid () const ;

        int take_handle();

        int handle() const ;

    private :

        void resume(int fd);

    private :
        int m_fd;
    };

    
}
