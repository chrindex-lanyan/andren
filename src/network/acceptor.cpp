

#include "acceptor.hh"
#include "eventloop.hh"
#include <cassert>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>



namespace  chrindex::andren::network
{
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
        if (onAccept){
            data->m_onAccept = onAccept;
        }
    }

    bool Acceptor::start(base::Socket && listenSock)
    {
        data->m_sock = std::move(listenSock);

        auto rp = data->m_wep.lock();
        if (!rp)
        {
            return false;
        }
        int fd = data->m_sock.handle();
        int events_listen = EPOLLIN ;
        bool bret ;

        auto ev = rp->eventLoopReference().lock();
        if(!ev)
        {
            return false;
        }
        bret = ev->addTask([ rp, fd ,events_listen, self = shared_from_this()]()
        {
            bool bret ;
            std::weak_ptr<Acceptor> wacceptor = self;
            bret = rp->setReadyCallback(fd, 
                [ wacceptor ,events_listen ](int events)
            {
                auto self = wacceptor.lock();
                if (!self)
                {
                    return ;
                }
                if (!(events & events_listen))
                {
                    self->data->m_onAccept({});
                    return ;
                }
                sockaddr_storage ss;
                uint32_t len;
                base::Socket cli = self->data->m_sock.accept(reinterpret_cast<sockaddr*>(&ss), &len);
                if (cli.valid())
                {
                    self->data->m_onAccept(std::make_shared<SockStream>(std::move(cli), self->data->m_wep));
                }
            });
            rp->append(fd, events_listen);
            assert(bret);
        },EventLoopTaskType::IO_TASK);

        return bret;
    }

    bool Acceptor::asyncStop(std::function<void()> onStop)
    {
        auto ev = data->m_wev.lock();
        if (!ev)
        {
            return false;
        }
        return ev->addTask([ self = shared_from_this(), cb = std::move(onStop)]()
        {
            auto ep = self->data->m_wep.lock();
            int fd = self->data->m_sock.handle();
            if(ep && fd >0)
            {
                ep->cancle(fd);
            }
            cb();
        },EventLoopTaskType::IO_TASK);
    }


}


