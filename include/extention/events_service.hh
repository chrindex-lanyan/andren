﻿#pragma once 

#include <cstdint>
#include <functional>

#include "kvpair.hpp"

namespace chrindex::andren::network
{
    class EventsService
    {
    public :
        EventsService(uint64_t key);
        EventsService(EventsService && ) noexcept;
        virtual ~EventsService();

        void operator =(EventsService &&) noexcept;

        using Processor = std::function<void (std::vector<base::kvpair<uint64_t, int>> & )>;
        using EventsHandler = std::function<void (uint64_t key , int events)>;

        void processEvents();

        uint64_t getKey () const;
    
    protected:
    
        void setNotifier(Processor && processor);
        void setEventsHandler(EventsHandler && handler);

    private :

        uint64_t m_key;
        Processor m_processor;
        EventsHandler m_handler;

    };
    

}