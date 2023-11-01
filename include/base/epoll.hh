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
    using event_contain = EventContain;

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

        /// @brief 
        /// @param op 
        /// @param fd 
        /// @param ev 
        /// @return 0为成功 
        int control(EpollCTRL op, int fd, epoll_event const & ev);

        /// @brief 
        /// @param ec 
        /// @param timeoutMsec 
        /// @return <0 为失败， =0 为没有， >0为事件数量
        int wait(EventContain &ec,  int timeoutMsec);

        /// @brief 
        /// @param ec 
        /// @param timeoutMsec 
        /// @param sigmask 
        /// @return <0 为失败， =0 为没有， >0为事件数量
        int pwait(EventContain &ec,  int timeoutMsec,sigset_t const * sigmask);

        /// @brief 
        /// @param ec 
        /// @param timeout 
        /// @param sigmask 
        /// @return <0 为失败， =0 为没有， >0为事件数量
        int pwait2(EventContain &ec, 
                   struct timespec const * timeout,
                   sigset_t const * sigmask);

        static int lastErrno() ;

        bool valid ()const ;

    private:
        int m_efd;
    };
    using linux_epoll = Epoll;

    struct EpollFDCreator
    {
        static int createSocketFD(int domain, int type, int protocol);

        static int createSignalFD(int fd, const sigset_t *mask, int flags);

        static int createTimerFD(int clockid, int flags);

        static int createEventFD(unsigned int initval, int flags);
    };
    using epoll_fd_creator = EpollFDCreator;

}