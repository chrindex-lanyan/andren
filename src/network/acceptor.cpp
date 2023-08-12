

#include "acceptor.hh"
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>



namespace  chrindex::andren::network
{
    Acceptor::Acceptor(){}

    Acceptor::Acceptor(std::weak_ptr<EventLoop> wev , std::weak_ptr<RePoller> wep)
    {
        data = std::make_unique<_private>();
        data->m_wev = wev;
        data->m_wep = wep;
    }

    Acceptor::~Acceptor(){}

    void Acceptor::setOnAccept(OnAccept const & onAccept)
    {
        OnAccept oa = onAccept;
        setOnAccept(std::move(oa));
    }

    void Acceptor::setOnAccept(OnAccept && onAccept)
    {
        if (data && onAccept){
            data->m_onAccept = onAccept;
        }
    }

    bool Acceptor::start(base::Socket && listenSock)
    {
        if (!data){
            return false;
        }
        data->m_sock = std::move(listenSock);

        auto rp = data->m_wep.lock();
        if (!rp)
        {
            return false;
        }
        int fd = data->m_sock.handle();
        int events_listen = EPOLLIN ;

        rp->append(fd, events_listen , 
            [this, self = shared_from_this() ,events_listen ](int events)
        {
            if (!(events & events_listen))
            {
                return ;
            }
            sockaddr_storage ss;
            uint32_t len;
            base::Socket cli = data->m_sock.accept(reinterpret_cast<sockaddr*>(&ss), &len);
            if (cli.valid())
            {
                data->m_onAccept(std::make_shared<SockStream>(std::move(cli), data->m_wep));
            }
        });

        return true;
    }


}


