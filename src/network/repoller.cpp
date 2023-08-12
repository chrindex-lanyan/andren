
#include "repoller.hh"
#include <memory>

namespace chrindex::andren::network
{
    RePoller::RePoller()
    {
        data = std::make_unique<_private>();
    }

    RePoller::~RePoller(){}


    void RePoller::append( int fd , int events , OnEventUP onEventUP)
    {

    }

    void RePoller::modify( int fd , int events , OnEventUP onEventUP)
    {

    }

    void RePoller::cancle( int fd , OnEventUP onEventUP)
    {
        
    }

    bool RePoller::start(std::weak_ptr<EventLoop> wev)
    {
        if (!data)
        {
            return false;
        }
        data->m_wev = wev;
        return true;
    }

    std::weak_ptr<EventLoop> RePoller::eventLoopReference() const
    {
        if (data)
        {
            return data->m_wev;
        }
        return {};
    }

    void RePoller::work(){}


}