#pragma once 

#include "../base/andren_base.hh"
#include "events_service.hh"
#include <functional>


namespace chrindex::andren::network
{

    class IOService : public EventsService ,base::noncopyable
    {
    public :

        IOService ();
        IOService (IOService && ios) noexcept;
        ~IOService();

        void operator= (IOService && ios) noexcept;

    private :
        std::map<int , std::any> m_fds_context;
        int m_uring;
    };

}