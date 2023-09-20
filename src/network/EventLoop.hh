#pragma once

#include "../base/andren_base.hh"
#include "events_service.hh"
#include "executor.hh"
#include <functional>
#include <future>
#include <map>
#include <thread>
#include <unordered_map>


namespace chrindex::andren::network
{

    class EventLoop :public base::noncopyable
    {
    public :
        EventLoop(uint32_t nthread = std::thread::hardware_concurrency());
        EventLoop(EventLoop &&) = delete;
        EventLoop(Executor && _move_executor);
        ~EventLoop ();

        void operator=(EventLoop && ev) = delete;
        
        bool operator==(EventLoop && ev) const noexcept;

        void addService(EventsService * _service);

        void delService(int64_t service_key, 
            std::function<void(EventsService * _refernce_service)> before);

        void modService(EventsService * _move_service);

        void findService(int64_t service_key, std::function<void(EventsService * _refernce_service)> after);

        uint32_t threadCount()const;

        bool start();

        bool shutdown();

    private :

        bool startNextStep();

        void addServiceConfigTask(uint32_t threadindex ,std::function<void(uint32_t index)> task, bool asap);

        void processEvents(uint32_t index);

    private :
        Executor m_exec;
        
        struct _Private_Thread_Local
        {
            std::map<int64_t, EventsService *> m_services;
        };

        _Private_Thread_Local * m_pdata;
        std::atomic<bool> m_shutdown;
    };


}