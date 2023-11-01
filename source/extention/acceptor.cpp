
#include <cassert>
#include <cstdio>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "acceptor.hh"
#include "task_distributor.hh"

namespace  chrindex::andren::network
{
    Acceptor::Acceptor(std::weak_ptr<TaskDistributor> wev , std::weak_ptr<RePoller> wep)
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
        if (onAccept)[[likely]]
        {
            data->m_onAccept = onAccept;
        }
    }

    bool Acceptor::start(base::Socket && listenSock)
    {
        data->m_sock = std::move(listenSock);

        auto rp = data->m_wep.lock();
        if (!rp)[[unlikely]]
        {
            return false;
        }
        int fd = data->m_sock.handle();
        int events_listen = EPOLLIN ;
        bool bret ;

        auto ev = rp->eventLoopReference().lock();
        if(!ev)[[unlikely]]
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
                if (!self)[[unlikely]]
                {
                    return ;
                }
                if (!(events & events_listen))[[likely]]
                {
                    self->data->m_onAccept({});
                    return ;
                }
                sockaddr_storage ss;
                uint32_t len = sizeof(ss);
                base::Socket cli = self->data->m_sock.accept(reinterpret_cast<sockaddr*>(&ss), &len);
                if (cli.valid())[[likely]]
                {
                    self->data->m_onAccept(std::make_shared<SockStream>(std::move(cli), self->data->m_wep));
                }
            });
            bret = rp->append(fd, events_listen);
            assert(bret);
        },TaskDistributorTaskType::IO_TASK);

        return bret;
    }

    bool Acceptor::asyncStop(std::function<void()> onStop)
    {
        auto ev = data->m_wev.lock();
        if (!ev)[[unlikely]]
        {
            return false;
        }
        return ev->addTask([ self = shared_from_this(), cb = std::move(onStop)]()
        {
            auto ep = self->data->m_wep.lock();
            int fd = self->data->m_sock.handle();
            if(ep && fd >0)[[unlikely]]
            {
                ep->cancle(fd);
            }
            cb();
        },TaskDistributorTaskType::IO_TASK);
    }


}


