#pragma once

#include <stdint.h>
#include <vector>

#include <sys/epoll.h>

#include "noncopyable.hpp"

namespace chrindex ::andren::base
{
    enum class EpollSupport : int
    {
        SOCKET_FD,
        TIMER_FD,
        SIGNAL_FD,
        EVENT_FD,
    };

    struct EventContain
    {
    public:
        EventContain(int defaultSize = 1024) { m_events.resize(defaultSize); }
        ~EventContain() {}

        size_t size() const { return m_events.size(); }

        epoll_event *reference_ptr(size_t pos)
        {
            if (pos >= size())
            {
                return 0;
            }
            return &m_events[pos];
        }

    private:
        std::vector<epoll_event> m_events;
    };

    enum class EpollCTRL : int
    {
        ADD = EPOLL_CTL_ADD,
        DEL = EPOLL_CTL_DEL,
        MOD = EPOLL_CTL_MOD,
    };

    class Epoll : public noncopyable
    {
    public:
        Epoll();

        ~Epoll();

        Epoll(Epoll &&);

        Epoll &operator=(Epoll &&);

        int control(EpollCTRL op, int fd, epoll_event const & ev);

        int wait(EventContain &ec, int maxevents, int timeoutMsec);

        int pwait(EventContain &ec, int maxevents, int timeoutMsec,
                  sigset_t const * sigmask);

        int pwait2(EventContain &ec, int maxevents,  
                   struct timespec const * timeout,
                   sigset_t const * sigmask);

        static int lastErrno() ;

        bool valid ()const ;

    private:
        int m_efd;
    };

    struct EpollFDCreator
    {
        static int createSocketFD(int domain, int type, int protocol);

        static int createSignalFD(int fd, const sigset_t *mask, int flags);

        static int createTimerFD(int clockid, int flags);

        static int createEventFD(unsigned int initval, int flags);
    };

}