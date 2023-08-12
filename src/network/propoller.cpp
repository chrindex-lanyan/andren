
#include "propoller.hh"

namespace chrindex::andren::network
{

    ProPoller::ProPoller()
    {
        m_exit = false;
    }

    ProPoller::~ProPoller()
    {
        m_exit = false;
    }

    bool ProPoller::valid() const
    {
        return !m_exit;
    }

    bool ProPoller::start(std::shared_ptr<EventLoop> ev)
    {
        if (!ev)
        {
            return false;
        }
        std::weak_ptr<EventLoop> wev = ev;
        m_exit = false;
        bool bret = ev->addTask([this , self = shared_from_this() , wev]()
                    { processEvents(wev); },
                    EventLoopTaskType::IO_TASK);
        m_exit = !bret;
        return bret;
    }

    bool ProPoller::addEvent(int fd, int events)
    {
        if (!m_epoll.valid() || fd <=0 || events <0)
        {
            return false;
        }
        m_cache[fd] = events;
        return realUpdateEvents(fd, events, base::EpollCTRL::ADD);
    }

    bool ProPoller::modEvent(int fd, int events)
    {
        if (!m_epoll.valid() || fd <=0 || events <0)
        {
            return false;
        }
        if (auto iter = m_cache.find(fd); iter != m_cache.end())
        {
            m_cache[fd] = events;
            return realUpdateEvents(fd, events, base::EpollCTRL::MOD);
        }
        return addEvent(fd,events);
    }

    bool ProPoller::delEvent(int fd)
    {
        int events = 0;
        m_cache.erase(fd);
        return realUpdateEvents(fd, events, base::EpollCTRL::DEL);
    }

    int ProPoller::findEvent(int fd)
    {
        if (auto iter = m_cache.find(fd); iter != m_cache.end())
        {
            return iter->second;
        }
        return -1;
    }

    int ProPoller::findLastEvent(int fd)
    {
        if (auto iter = m_lastWait.find(fd); iter != m_lastWait.end())
        {
            return iter->second;
        }
        return -1;
    }

    bool ProPoller::findAndWait(int fd, int events, int timeoutMsec , EventLoop* wev  , OnFind cb)
    {
        if (!wev || !m_epoll.valid() || !cb)
        {
            return false;
        }

        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::system_clock().now().time_since_epoch()).count();

        int _events = findEvent(fd);

        if ( _events < 0)
        {
            cb(ProPollerError::FD_NOTFOUND_IN_CACHE);
            return false;
        }
        if ( !(_events & events))
        {
            cb(ProPollerError::FD_FOUND_BUT_EVENT_NOFOUND);
            return false;
        }

        int last = findLastEvent(fd);
        if ( (last > 0) && (last & events) )
        {
            cb(ProPollerError::OK);
            return true;
        }
        
        if (timeoutMsec <=0 )
        {
            cb(ProPollerError::FD_AND_EVENT_FOUND_BUT_NO_OCCURRED);
            return false;
        }

        wev->addTask([self = shared_from_this() , this , now , fd, 
                events, timeoutMsec , wev , cb = std::move(cb)]()mutable
        {
            int64_t crrt =  std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::system_clock().now().time_since_epoch()).count();
            timeoutMsec = timeoutMsec + now - crrt;
            findAndWait(fd,events,timeoutMsec,wev,std::move(cb));
        },EventLoopTaskType::IO_TASK);

        return true;
    }

    bool ProPoller::realUpdateEvents(int fd, int events, base::EpollCTRL ctrl)
    {
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        //fprintf(stdout, "ProPoller::realUpdateEvents   :   fd = %d , events = %d.\n",fd,events);
        return 0 == m_epoll.control(ctrl,fd,ev);
    }

    bool ProPoller::processEvents(std::weak_ptr<EventLoop> wev)
    {
        auto ev = wev.lock();
        if (!ev || !m_epoll.valid())
        {
            return false;
        }
        
        base::EventContain ec(300);
        int num ;

        num = m_epoll.wait(ec, 2);

        if (num<0) // wait failed
        {
            return false;
        }
        else if(num ==0) // timeout
        {
            //
        }

        m_lastWait.clear();
        for (int i =0 ; i< num ; i++)
        {
            auto pevent = ec.reference_ptr(i);
            m_lastWait[pevent->data.fd] = pevent->events;
            //fprintf(stdout,"ProPoller::processEvents : Epoll Wait Some . FD =%d , Events =%d.\n",pevent->data.fd , pevent->events);
        }
        
        if (m_exit)
        {
            return true;
        }
        ev->addTask([this, wev , self = shared_from_this()]()
                    { processEvents(wev); },
                    EventLoopTaskType::IO_TASK);
        return true;
    }

}