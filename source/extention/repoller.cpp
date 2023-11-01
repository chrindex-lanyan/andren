
#include "repoller.hh"
#include "task_distributor.hh"
#include <memory>
#include <unistd.h>

namespace chrindex::andren::network
{
    RePoller::RePoller()
    {
        data = std::make_unique<_private>();
    }

    RePoller::~RePoller()
    {
        stop();
    }


    bool RePoller::append( int fd , int events )
    {
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        return 0 == data->m_ep.control(base::EpollCTRL::ADD,  fd, ev);
    }

    bool RePoller::modify( int fd , int events )
    {
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        return 0 == data->m_ep.control(base::EpollCTRL::MOD,  fd, ev);
    }

    bool RePoller::cancle( int fd )
    {
        data->m_callbacks.erase(fd);
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = 0;
        return 0 == data->m_ep.control(base::EpollCTRL::DEL,  fd, ev);
    }

    bool RePoller::notifyEvents(int fd , int events)
    {
        if( fd == 0 || fd == -1){return false;}
        if (auto iter = data->m_callbacks.find(fd); iter != data->m_callbacks.end())
        {
            data->m_customEvents[fd] = events;
            return true;
        }
        return false;
    }

    bool RePoller::setReadyCallback(int fd , OnEventUP const & onEventUP)
    {
        auto cb = onEventUP;
        return setReadyCallback(fd, std::move(cb));
    }

    bool RePoller::setReadyCallback(int fd , OnEventUP && onEventUP)
    {
        if (!onEventUP|| fd==0 || fd == -1) // 不允许以下值：0,-1
        {
            return false;
        }
        data->m_callbacks[fd] = std::move(onEventUP);
        return true;
    }

    bool RePoller::start(std::weak_ptr<TaskDistributor> wev, int epollWaitPerTick_msec)
    {
        auto ev = wev.lock();
        if (!ev)
        {
            return false;
        }
        data->m_wev = wev;
        data->m_shutdown = false;
        workNextTick(epollWaitPerTick_msec);
        return true;
    }

    void RePoller::stop()
    {
        data->m_shutdown = true;
    }

    std::weak_ptr<TaskDistributor> RePoller::eventLoopReference() const
    {
        if (data)
        {
            return data->m_wev;
        }
        return {};
    }

    void RePoller::workNextTick(int timeoutMsec)
    {
        auto ev = data->m_wev.lock();
        ev->addTask([timeoutMsec, this, self = shared_from_this()]()
        {
            work(timeoutMsec);
            if(false == data->m_shutdown)
            {
                workNextTick(timeoutMsec);
            }
        },TaskDistributorTaskType::IO_TASK);
    }

    void RePoller::work(int timeoutMsec)
    {
        base::EventContain ec(300);
        std::map<int,int> currentEventsMap = std::move(data->m_customEvents);// 查自定义Events
        int num ;
        int numEvMap = currentEventsMap.size();
        int numCbMap = data->m_callbacks.size();

        // 查系统FD的events
        int usedTimeoutMsec = (numEvMap > 0) ? (1) : (timeoutMsec); // 如果有自定义events要处理，就选择最小wait时间。
        num = data->m_ep.wait(ec, usedTimeoutMsec); // N Msec Per Tick

        if (num < 0 && errno != EINTR) [[unlikely]] // wait failed
        {
            //fprintf(stderr,"RePoller::work():: wait failed, errno = %m. exit...\n",errno);
            data->m_shutdown = true;
            return ;
        }
        else if(num == 0 ) // timeout
        {
            //ignore
            //fprintf(stderr,"RePoller::work():: timeout...msec = %d\n",usedTimeoutMsec);
        }
        else 
        { 
            // ignore
            //fprintf(stdout,"EPOLL Wait Some(%d).\n" ,num ); 
        }

        // 整合Events
        for (int i = 0 ; i < num ; i++)
        {
            auto pevent = ec.reference_ptr(i);
            if (auto iter = currentEventsMap.find(pevent->data.fd); iter != currentEventsMap.end())
            {
                int events = iter->second;
                iter->second = events | pevent->events;
            }
            else 
            {
                currentEventsMap[pevent->data.fd] = pevent->events;
            }
        }

        /// 回调和统一处理
        /// 判断遍历哪一个。通常遍历小的那个map更快。
        if(numEvMap < numCbMap)
        {
            for (auto & eviter: currentEventsMap )
            {
                int fd = eviter.first;
                int events = eviter.second;
                //fprintf(stdout,"RePoller::work():: Process fd=%d,events=%d ...\n",fd,events);
                if (auto iter = data->m_callbacks.find(fd) ; iter != data->m_callbacks.end())
                {
                    iter->second(events);
                }
            }
        }
        else // if(numEvMap >= numCbMap)
        {
            for(auto & iter : data->m_callbacks)
            {
                int fd = iter.first;
                auto & cb = iter.second;
                if(auto eviter = currentEventsMap.find(fd); eviter != currentEventsMap.end())
                {
                    cb(eviter->second);
                }
            }
        }
    }

    bool RePoller::saveObject(int id , bool force , std::any _object,  std::function<void(bool ret, std::any * _obj)> onSave)
    {
        if(!onSave )
        {
            return false;
        }
        
        auto ev = data->m_wev.lock();
        if (!ev)
        {
            return false;
        }
        return ev->addTask([ this, self = shared_from_this(), 
            id, force , obj = std::move(_object), cb = std::move(onSave) ]()mutable
        {
            auto iter = data->m_objects.find(id);
            if(iter != data->m_objects.end()) // 如果查到有重复
            {
                if(force) // 强制覆盖
                {
                    iter->second = std::move(obj);
                    cb(true,&iter->second);
                }
                else  // 不强制覆盖
                {
                    cb(false, &obj);
                }
            }else  // 如果没有重复，直接保存
            {
                cb(true,&obj);
                data->m_objects[id] =  std::move(obj);
            }
        },TaskDistributorTaskType::IO_TASK);
    }


    bool RePoller::findObject(int id , bool takeOrNot ,  std::function<void(bool ret, std::any * _obj)> onFind )
    {
        auto ev = data->m_wev.lock();
        if (!ev)
        {
            return false;
        }
        return ev->addTask([id,takeOrNot, this, self = shared_from_this() , cb = std::move(onFind)]()
        {
            auto iter = data->m_objects.find(id);
            if(iter != data->m_objects.end()) // 查到了
            {
                std::any _tmp;
                if (takeOrNot)  // 要取出
                {
                    _tmp = std::move(iter->second);
                    data->m_objects.erase(iter);
                    if (cb)
                    {
                        cb(true, &_tmp);
                    }
                }
                else // 不取出
                {
                    if (cb)
                    {
                        cb(true, &iter->second);
                    }
                }
            }else // 没查到 
            {
                if(cb)
                {
                    cb(false , nullptr ); 
                }
            }
        },TaskDistributorTaskType::IO_TASK);
    }


}