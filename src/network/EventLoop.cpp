
#include"EventLoop.hh"
#include "events_service.hh"
#include "executor.hh"
#include "schedule.hh"
#include <future>
#include <memory>
#include <thread>
#include <utility>

namespace chrindex::andren::network
{

    EventLoop::EventLoop(uint32_t nthread)
    {
        m_shutdown = false;
        m_exec = Executor(nthread);
        for (uint32_t i = 0; i < nthread ; i++)
        {
            m_pdata.push_back(std::make_shared<_Private_Thread_Local>());
        }
    }

    EventLoop::EventLoop(Executor && _move_executor)
    {
        m_shutdown = false;
        m_exec = std::move(_move_executor);
        for (uint32_t i = 0; i < m_exec.threadCount() ; i++)
        {
            m_pdata.push_back(std::make_shared<_Private_Thread_Local>());
        }
    }

    EventLoop::~EventLoop ()
    {
        m_shutdown = true;
    }
    
    bool EventLoop::operator==(EventLoop && el) const noexcept
    {
        return this == &el;
    }

    bool EventLoop::start()
    {
        if (!m_exec.valid() || !m_pdata.empty())
        {
            return false;
        }
        m_shutdown = false;
        return startNextStep();
    }

    bool EventLoop::shutdown()
    {
        m_shutdown = true;
        return true;
    }

    bool EventLoop::startNextStep()
    {
        for (uint32_t index = 0; index < m_exec.threadCount(); index++)
        {
            bool bret= m_exec.addTask(index,[this](Executor * , uint32_t index)
            {
                processEvents(index);
            });
            if (!bret)
            {
                return false;
            }
        }
        return true;
    }

    void EventLoop::processEvents(uint32_t index)
    {
        auto context = m_pdata[index];
        for (auto const & service: context->m_services)
        {
            service.second->processEvents();
        }
        startNextStep();
    }

    void EventLoop::addService(EventsService * _service)
    {
        if (!_service || !m_exec.valid() || m_pdata.empty()) [[unlikely]]
        {
            return ;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(_service->getKey());
        addServiceConfigTask(index, [this, _service](uint32_t index)
        {
            m_pdata[index]->m_services[_service->getKey()] = _service;
        },true);
    }

    void EventLoop::delService(int64_t service_key,
            std::function<void(EventsService * _refernce_service)> before)
    {
        if (!m_exec.valid() || m_pdata.empty()) [[unlikely]]
        {
            return ;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(service_key);
        addServiceConfigTask(index, [this, service_key, be = std::move(before)](uint32_t index)
        {
            auto iter = m_pdata[index]->m_services.find(service_key);
            if (be) 
            { 
                be(iter->second); 
            }
            m_pdata[index]->m_services.erase(iter);
        },true);
    }

    void EventLoop::modService(EventsService * _move_service)
    {
        if (!m_exec.valid() || m_pdata.empty() || !_move_service) [[unlikely]]
        {
            return ;
        }
        int64_t service_key = _move_service->getKey();
        uint32_t index = Schedule(threadCount()).doSchedule(service_key);
        addServiceConfigTask(index, [this, _move_service, service_key](uint32_t index)
        {
            m_pdata[index]->m_services[service_key] = _move_service;
        },true);
    }

    void EventLoop::findService(int64_t service_key, std::function<void(EventsService * _refernce_service)> after)
    {
        if (!m_exec.valid() || m_pdata.empty()) [[unlikely]]
        {
            return ;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(service_key);

        addServiceConfigTask(index, [t = std::move(after), this,service_key](uint32_t index)
        {
            EventsService * ptr = nullptr;
            auto iter = m_pdata[index]->m_services.find(service_key);
            if (iter != m_pdata[index]->m_services.end())
            {
                ptr = iter->second;
            }
            t(ptr);
        } , true);
    }

    void EventLoop::addServiceConfigTask(uint32_t threadindex ,std::function<void(uint32_t index)> task, bool asap)
    {
        if (asap)
        {
            m_exec.addTask_ASAP(threadindex,[ t = std::move(task) ](Executor *, uint32_t index)
            {
                t(index);
            });
        }
        else 
        {
            m_exec.addTask(threadindex,[ t = std::move(task) ](Executor *, uint32_t index)
            {
                t(index);
            });
        }
    }

    uint32_t EventLoop::threadCount()const
    {
        return m_exec.threadCount();
    }
    
}

