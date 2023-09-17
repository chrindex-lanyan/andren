
#include"EventLoop.hh"
#include "schedule.hh"
#include <utility>

namespace chrindex::andren::network
{

    EventLoop::EventLoop()
    {
        m_pdata =0;
    }

    EventLoop::EventLoop(EventLoop && el) noexcept
    {
        m_exec = std::move(el.m_exec);
        m_pdata = std::move(el.m_pdata);
        el.m_pdata =0;
    }

    EventLoop::EventLoop(Executor && _move_executor)
    {
        m_exec = std::move(_move_executor);
        m_pdata = new _Private_Thread_Local[  m_exec.threadCount() ];
    }

    EventLoop::~EventLoop ()
    {
        if (m_pdata){
            delete [] m_pdata;
        }
    }

    void EventLoop::operator=(EventLoop && el) noexcept
    {
        this->~EventLoop();
        m_exec = std::move(el.m_exec);
        m_pdata = std::move(el.m_pdata);
    }
    
    bool EventLoop::operator==(EventLoop && el) const noexcept
    {
        return this == &el;
    }

    void EventLoop::processEvents()
    {
        //
    }

    void EventLoop::addService(EventsService * _move_service)
    {
        if (!_move_service || !m_exec.valid() || !m_pdata) [[unlikely]]
        {
            return ;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(_move_service->getKey());

        m_pdata[index].m_services[_move_service->getKey()] = _move_service;
    }

    void EventLoop::delService(int64_t service_key,
            std::function<void(EventsService * _refernce_service)> before)
    {
        if (!m_exec.valid() || !m_pdata) [[unlikely]]
        {
            return ;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(service_key);
        auto iter = m_pdata[index].m_services.find(service_key);
        if (before) { before(iter->second); }
        m_pdata[index].m_services.erase(iter);
    }

    void EventLoop::modService(EventsService * _move_service)
    {
        if (!m_exec.valid() || !m_pdata) [[unlikely]]
        {
            return ;
        }
    }

    void EventLoop::findService(int64_t service_key)
    {

    }

    uint32_t EventLoop::threadCount()const
    {
        return m_exec.threadCount();
    }
    
}

