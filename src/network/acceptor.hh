#pragma once

#include "../base/andren_base.hh"
#include <functional>
#include <memory>


#include "eventloop.hh"
#include "sockstream.hh"
#include "repoller.hh"

namespace chrindex::andren::network
{

    class Acceptor : public std::enable_shared_from_this<Acceptor> , base::noncopyable
    {
    public :

        Acceptor();

        Acceptor(std::weak_ptr<EventLoop> wev , std::weak_ptr<RePoller> wep);

        ~Acceptor();

        using OnAccept = std::function<void(std::shared_ptr<SockStream> clis)>;


        void setOnAccept(OnAccept const & onAccept);

        void setOnAccept(OnAccept && onAccept);

        bool start(base::Socket && listenSock);


    private :

        struct _private 
        {
            OnAccept m_onAccept;
            std::weak_ptr<EventLoop> m_wev;
            std::weak_ptr<RePoller> m_wep;
            base::Socket m_sock;
        };
        std::unique_ptr<_private> data;

    };


}

