#pragma once

#include "../base/andren_base.hh"

#include "eventloop.hh"



namespace chrindex::andren::network
{
    
    class UdpStream : public std::enable_shared_from_this<UdpStream> , base::noncopyable
    {
    public :
        UdpStream();
        UdpStream(UdpStream && _);
        UdpStream(base::Socket && sock);
        ~UdpStream();

        void operator=(UdpStream && _);

        void setEventLoop(std::weak_ptr<EventLoop> ev);

        void setEpoller(std::weak_ptr<base::Epoll> ep);

        bool bind();

        bool start();

        bool valid () const;

        base::Socket * handle();

        bool requestRead();

        bool requestWrite(std::string const & data, base::EndPointIPV4 const & epv4);

        bool requestWrite(std::string && data, base::EndPointIPV4 && epv4);

    private:

        void processEvents();


    private:
        
        struct _Private 
        {
            base::Socket m_sock;
        } ;

        std::unique_ptr<_Private> pdata;
    };




}

