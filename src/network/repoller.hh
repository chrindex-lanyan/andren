#pragma once

#include "../base/andren_base.hh"

#include "eventloop.hh"
#include <functional>
#include <map>
#include <memory>


namespace chrindex::andren::network
{

    class RePoller : public std::shared_ptr<RePoller> , base::noncopyable
    {
    public :
        RePoller();
        ~RePoller();

        using OnEventUP = std::function<void(int events)>;

        void append( int fd , int events , OnEventUP onEventUP);

        void modify( int fd , int events , OnEventUP onEventUP);

        void cancle( int fd , OnEventUP onEventUP);

        bool start(std::weak_ptr<EventLoop> wev);

        std::weak_ptr<EventLoop> eventLoopReference() const;

    private:

        void work();

    private :

        struct _private 
        {
            std::map<int, OnEventUP> m_callbacks;
            base::Epoll m_ep;
            std::weak_ptr<EventLoop> m_wev;
        };

        std::unique_ptr<_private> data;

    };




}