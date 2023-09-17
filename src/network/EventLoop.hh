#pragma once

#include "../base/andren_base.hh"
#include "events_service.hh"
#include "executor.hh"
#include <functional>
#include <map>
#include <unordered_map>


namespace chrindex::andren::network
{

    class EventLoop :public base::noncopyable
    {
    public :
        EventLoop();
        EventLoop(EventLoop &&) noexcept;
        EventLoop(Executor && _move_executor);
        ~EventLoop ();

        void operator=(EventLoop && ev) noexcept;
        
        bool operator==(EventLoop && ev) const noexcept;

        void processEvents();

        void addService(EventsService * _move_service);

        void delService(int64_t service_key, 
            std::function<void(EventsService * _refernce_service)> before);

        void modService(EventsService * _move_service);

        void findService(int64_t service_key);

        uint32_t threadCount()const;
    
    private :
        Executor m_exec;
        
        struct _Private_Thread_Local
        {
            std::map<int64_t, EventsService *> m_services;
        };

        _Private_Thread_Local * m_pdata;        
    };


}