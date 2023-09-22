#pragma once

#include "../base/andren_base.hh"
#include "events_service.hh"
#include "executor.hh"
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <thread>
#include <unordered_map>


namespace chrindex::andren::network
{

    class EventLoop :public base::noncopyable
    {
    public :
        EventLoop();
        EventLoop(uint32_t nthread );
        EventLoop(EventLoop &&) noexcept;
        EventLoop(Executor && _move_executor);
        ~EventLoop ();

        void operator=(EventLoop && ev) noexcept;
        
        bool operator==(EventLoop && ev) const noexcept;

        bool addService(EventsService * _service);

        bool delService(int64_t service_key, 
            std::function<void(EventsService * _refernce_service)> before);

        bool modService(EventsService * _move_service);

        bool findService(int64_t service_key, std::function<void(EventsService * _refernce_service)> after);

        uint32_t threadCount()const;

        bool start();

        bool shutdown();

    private :

        struct _private_data ;

        bool startNextStep(uint32_t index);

        void addServiceConfigTask(uint32_t threadindex ,std::function<void(uint32_t index)> task, bool asap);

        void processEvents(uint32_t index, _private_data * p_data);

    private :

        struct _Private_Thread_Local
        {
            std::map<int64_t, EventsService *> m_services;
        };

        struct _private_data 
        {
            _private_data ()
            {
                printf("eventloop :: _private_data 构造.\n");
            }
            ~_private_data ()
            {
                printf("eventloop :: _private_data 析构.\n");
                m_exec = Executor{};
                m_pdata.clear();
            }
            
            std::vector<std::shared_ptr<_Private_Thread_Local>> m_pdata;
            Executor m_exec;
        };
        std::shared_ptr<_private_data> data;
        
    };


}