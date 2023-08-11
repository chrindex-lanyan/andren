
#include "propoller.hh"

namespace chrindex::andren::network
{

    ProEvent::ProEvent()
    {
        readable = 0;
        writeable = 0;
        except = 0;
        reserv = 0;
    }

    void ProEvent::cover(int32_t epoll_event_events)
    {
        if (epoll_event_events & EPOLLIN)
        {
            readable = 1;
        }
        if (epoll_event_events & EPOLLOUT)
        {
            writeable = 1;
        }
        if (epoll_event_events & EPOLLERR)
        {
            except = 1;
        }
    }

    int32_t ProEvent::cover()
    {
        int ret = 0;
        if (readable)
        {
            ret |= EPOLLIN;
        }
        if (writeable)
        {
            ret |= EPOLLOUT;
        }
        if (except)
        {
            ret |= EPOLLERR;
        }
        return ret;
    }

    ProPoller::ProPoller()
    {
        //
    }

    ProPoller::~ProPoller()
    {
        //
    }

    void ProPoller::setEventLoop(std::weak_ptr<EventLoop> ev)
    {
        m_ev = ev;
    }

    void ProPoller::setEpoller(std::weak_ptr<base::Epoll> ep)
    {
        m_ep = ep;
    }

    std::weak_ptr<base::Epoll> ProPoller::epollHandle()
    {
        auto ep = m_ep.lock();
        if (!ep || !ep->valid())
        {
            return {};
        }
        return ep;
    }

    bool ProPoller::valid() const
    {
        auto ev = m_ev.lock();
        auto ep = m_ep.lock();
        if (!ev || !ep || !ep->valid())
        {
            return false;
        }
        return true;
    }

    bool ProPoller::start()
    {
        auto ev = m_ev.lock();
        auto ep = m_ep.lock();
        if (!ev || !ep || !ep->valid())
        {
            return false;
        }
        ev->addTask([self = shared_from_this()]()
                    { self->processEvents(); },
                    EventLoopTaskType::IO_TASK);
        return true;
    }

    bool ProPoller::subscribe(int fd, ProEvent proev)
    {
        return updateEpoll(fd, proev, base::EpollCTRL::ADD);
    }

    bool ProPoller::modSubscribe(int fd, ProEvent proev)
    {
        return updateEpoll(fd, proev, base::EpollCTRL::MOD);
    }

    bool ProPoller::delSubscribe(int fd)
    {
        m_cacheEvents.erase(fd);
        ProEvent proev;
        return updateEpoll(fd, proev, base::EpollCTRL::DEL);
    }

    void ProPoller::clearCache()
    {
        m_cacheEvents.clear();
    }

    void ProPoller::findAndWait(int32_t fd, std::function<void(ProEvent, bool isTimeout)> cb, int64_t timeoutMsec)
    {
        if (auto iter = m_cacheEvents.find(fd); (iter != m_cacheEvents.end() ) && cb)
        {
            cb(iter->second, false);
            return;
        }
        
        if ((timeoutMsec <= 0))
        {
            cb({}, true);
            return;
        }

        auto ev = m_ev.lock();
        if (!ev )
        {
            return;
        }
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock().now().time_since_epoch()).count();
        ev->addTask([fd, _cb = std::move(cb), timeoutMsec, now, self = shared_from_this()]() mutable
                    {
                        int64_t tmsec = std::chrono::duration_cast<std::chrono::milliseconds> 
                                (std::chrono::system_clock().now().time_since_epoch()).count(); 
                        tmsec = timeoutMsec - ( tmsec - now );
                        self->findAndWait(fd, std::move(_cb) , tmsec);
                    },
                    EventLoopTaskType::IO_TASK);
    }

    void ProPoller::processEvents()
    {
        auto ep = m_ep.lock();
        if (!ep || !ep->valid())
        {
            return;
        }
        int32_t number = ep->wait(m_evc, 1);
        if (number < 0) // faild
        {
            return;
        }
        else if (number == 0) // timeout
        {
            //
        }
        else
        {
            for (int i = 0; i < number; i++)
            {
                auto pevent = m_evc.reference_ptr(i);
                ProEvent proev;
                proev.cover(pevent->events);
                m_cacheEvents[pevent->data.fd] = proev;
            }
        }

        auto ev = m_ev.lock();
        if (!ev )
        {
            return;
        }
        ev->addTask([self = shared_from_this()]()
                    { self->processEvents(); },
                    EventLoopTaskType::IO_TASK);
    }

    bool ProPoller::updateEpoll(int fd, ProEvent proev, base::EpollCTRL type)
    {
        auto ev = m_ev.lock();
        auto ep = m_ep.lock();
        if (!ev || !ep || !ep->valid())
        {
            return false;
        }
        epoll_event eev;
        eev.data.fd = fd;
        eev.events = proev.cover();
        return 0 == ep->control(type, fd, eev);
    }
    
}