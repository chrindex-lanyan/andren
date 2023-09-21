﻿
#include"EventLoop.hh"
#include "events_service.hh"
#include "executor.hh"
#include "schedule.hh"
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <utility>

namespace chrindex::andren::network
{

    EventLoop::EventLoop(uint32_t nthread)
    {
        data = std::make_shared<_private_data>();
        data->m_shutdown = false;
        data->m_exec = Executor(nthread);
        for (uint32_t i = 0; i < nthread ; i++)
        {
            data->m_pdata.push_back(std::make_shared<_Private_Thread_Local>());
        }
    }

    EventLoop::EventLoop(Executor && _move_executor)
    {
        data = std::make_shared<_private_data>();
        data->m_shutdown = false;
        data->m_exec = std::move(_move_executor);
        for (uint32_t i = 0; i < data->m_exec.threadCount() ; i++)
        {
            data->m_pdata.push_back(std::make_shared<_Private_Thread_Local>());
        }
    }

    EventLoop::~EventLoop ()
    {
        if (data)
        {
            data->m_shutdown.exchange(true,std::memory_order_seq_cst);
        }
    }

    void EventLoop::operator=(EventLoop && oth) noexcept
    {
        this->~EventLoop();
        data = std::move(oth.data);
    }

    EventLoop::EventLoop(EventLoop && oth) noexcept
    {
        data = std::move(oth.data);
    }
    
    bool EventLoop::operator==(EventLoop && el) const noexcept
    {
        return this == &el;
    }

    bool EventLoop::start()
    {
        if (!data || !data->m_exec.valid() || data->m_pdata.empty())
        {
            return false;
        }
        data->m_shutdown = false;
        return startNextStep();
    }

    bool EventLoop::shutdown()
    {
        data->m_shutdown = true;
        return true;
    }

    bool EventLoop::startNextStep()
    {
        auto self_data = data;
        if (!self_data || self_data->m_shutdown)
        {
            return false;
        }
        for (uint32_t index = 0; index < self_data->m_exec.threadCount(); index++)
        {
            bool bret= self_data->m_exec.addTask(index,[this,self_data](Executor * , uint32_t index)
            {
                processEvents(index, self_data.get());
            });
            if (!bret)
            {
                return false;
            }
        }
        return true;
    }

    void EventLoop::processEvents(uint32_t index , _private_data * p_data)
    {
        auto context = p_data->m_pdata[index];
        for (auto const & service: context->m_services)
        {
            service.second->processEvents();
        }
        startNextStep();
    }

    bool EventLoop::addService(EventsService * _service)
    {
        if (!data || !_service || !data->m_exec.valid() || data->m_pdata.empty()) [[unlikely]]
        {
            return false;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(_service->getKey());
        addServiceConfigTask(index, [this, _service](uint32_t index)
        {
            data->m_pdata[index]->m_services[_service->getKey()] = _service;
        },true);
        return true;
    }

    bool EventLoop::delService(int64_t service_key,
            std::function<void(EventsService * _refernce_service)> before)
    {
        if (!data || !data->m_exec.valid() || data->m_pdata.empty()) [[unlikely]]
        {
            return false;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(service_key);
        addServiceConfigTask(index, [this, service_key, be = std::move(before)](uint32_t index)
        {
            auto iter = data->m_pdata[index]->m_services.find(service_key);
            if (be) 
            { 
                be(iter->second); 
            }
            data->m_pdata[index]->m_services.erase(iter);
        },true);
        return true;
    }

    bool EventLoop::modService(EventsService * _move_service)
    {
        if (!data || !data->m_exec.valid() || data->m_pdata.empty() || !_move_service) [[unlikely]]
        {
            return false;
        }
        int64_t service_key = _move_service->getKey();
        uint32_t index = Schedule(threadCount()).doSchedule(service_key);
        addServiceConfigTask(index, [this, _move_service, service_key](uint32_t index)
        {
            data->m_pdata[index]->m_services[service_key] = _move_service;
        },true);
        return true;
    }

    bool EventLoop::findService(int64_t service_key, std::function<void(EventsService * _refernce_service)> after)
    {
        if (!data || !data->m_exec.valid() || data->m_pdata.empty()) [[unlikely]]
        {
            return false;
        }
        uint32_t index = Schedule(threadCount()).doSchedule(service_key);

        addServiceConfigTask(index, [t = std::move(after), this,service_key](uint32_t index)
        {
            EventsService * ptr = nullptr;
            auto iter = data->m_pdata[index]->m_services.find(service_key);
            if (iter != data->m_pdata[index]->m_services.end())
            {
                ptr = iter->second;
            }
            t(ptr);
        } , true);
        return true;
    }

    void EventLoop::addServiceConfigTask(uint32_t threadindex ,std::function<void(uint32_t index)> task, bool asap)
    {
        if (asap)
        {
            data->m_exec.addTask_ASAP(threadindex,[ t = std::move(task) ](Executor *, uint32_t index)
            {
                t(index);
            });
        }
        else 
        {
            data->m_exec.addTask(threadindex,[ t = std::move(task) ](Executor *, uint32_t index)
            {
                t(index);
            });
        }
    }

    uint32_t EventLoop::threadCount()const
    {
        return data->m_exec.threadCount();
    }
    
}

