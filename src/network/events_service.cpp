#include "events_service.hh"

namespace chrindex::andren::network
{
    EventsService::EventsService(int64_t key)
    {
        m_key = key;
    }

    EventsService::EventsService(EventsService && es) noexcept
    {
        m_key = std::move(es.m_key);
        m_processor = std::move(es.m_processor);
        m_handler = std::move(es.m_handler);
    }  

    EventsService::~EventsService()
    {

    }

    void EventsService::operator =(EventsService && es) noexcept
    {
        this->~EventsService();
        m_key = std::move(es.m_key);
        m_processor = std::move(es.m_processor);
        m_handler = std::move(es.m_handler);
    }


    void EventsService::setNotifier(Processor && notifier)
    {
        m_processor = std::move(notifier);
    }

    void EventsService::setEventsHandler(EventsHandler && handler)
    {
        m_handler = std::move(handler);
    }

    void EventsService::processEvents()
    {
        if (m_processor)[[likely]]
        {
            std::vector<base::KVPair<int64_t, int>> result;
            m_processor(result);
            if (m_handler)[[likely]]
            {
                for (auto const & kv : result)
                {
                    m_handler(kv.key(),kv.value_from_copy());
                }
            }
        }
    }

    int64_t EventsService::getKey () const
    {
        return m_key;
    }

}
