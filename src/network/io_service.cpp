#include "io_service.hh"
#include "events_service.hh"


#include <liburing.h>


namespace chrindex::andren::network
{

    IOService::IOService ()
    {
        m_uring =0;
    }
    IOService::IOService (IOService && ios) noexcept : EventsService(std::move(ios))
    {
        m_fds_context = std::move(ios.m_fds_context);
        m_uring = std::move(ios.m_uring);
        ios.m_uring = 0;
    }

    IOService::~IOService()
    {
        if (m_uring>0)
        {
            
        }
    }

    void IOService::operator= (IOService && ios) noexcept
    {
        this->~IOService();
        EventsService::operator=(std::move(ios));
        m_fds_context = std::move(ios.m_fds_context);
        m_uring = std::move(ios.m_uring);
        ios.m_uring = 0;
    }

}

