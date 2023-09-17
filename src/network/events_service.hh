#pragma once 

#include "../base/andren_base.hh"
#include <cstdint>
#include <functional>


namespace chrindex::andren::network
{
    class EventsService
    {
    public :
        EventsService();
        EventsService(int64_t key);
        EventsService(EventsService && ) noexcept;
        virtual ~EventsService();

        void operator =(EventsService &&) noexcept;

        using Processor = std::function<void (std::vector<base::KVPair<int64_t, int>> & )>;
        using EventsHandler = std::function<void (int64_t key , int events)>;

        void processEvents();

        int64_t getKey () const;
    
    protected:
    
        void setNotifier(Processor && processor);
        void setEventsHandler(EventsHandler && handler);

    private :

        int64_t m_key;
        Processor m_processor;
        EventsHandler m_handler;

    };
    

}