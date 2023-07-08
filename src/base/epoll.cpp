

#include "epoll.hh"

#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <errno.h>
#include <assert.h>

namespace chrindex::andren::base
{
    Epoll::Epoll()
    {
        m_efd = 0;
        int efd = epoll_create(1);
        if (efd > 0)
        {
            m_efd = efd;
        }
    }
    Epoll::~Epoll()
    {
        if (m_efd > 0)
        {
            ::close(m_efd);
        }
    }

    Epoll::Epoll(Epoll &&_)
    {
        m_efd = _.m_efd;
        _.m_efd = 0;
    }
    Epoll &Epoll::operator=(Epoll &&_)
    {
        this->~Epoll();
        m_efd = _.m_efd;
        _.m_efd = 0;
        return *this;
    }

    int Epoll::control(EpollCTRL op, int fd, epoll_event const &ev)
    {
        return ::epoll_ctl(m_efd, static_cast<int>(op), fd, const_cast<epoll_event *>(&ev));
    }

    int Epoll::wait(EventContain &ec, int maxevents, int timeoutMsec)
    {
        return ::epoll_wait(m_efd, ec.reference_ptr(0), ec.size(), timeoutMsec);
    }

    int Epoll::pwait(EventContain &ec, int maxevents, int timeoutMsec, sigset_t const *sigmask)
    {
        return ::epoll_pwait(m_efd, ec.reference_ptr(0), ec.size(), timeoutMsec, sigmask);
    }

    int Epoll::pwait2(EventContain &ec, int maxevents, struct timespec const *timeout, sigset_t const *sigmask)
    {
        return ::epoll_pwait2(m_efd, ec.reference_ptr(0), ec.size(), timeout, sigmask);
    }

    int Epoll::lastErrno() { return errno; }

    bool Epoll::valid() const { return m_efd > 0; }

    int EpollFDCreator::createSocketFD(int domain, int type, int protocol)
    {
        return ::socket(domain,type,protocol);
    }

    int EpollFDCreator::createSignalFD(int fd, const sigset_t *mask, int flags)
    {
        return ::signalfd(fd,mask,flags);
    }

    int EpollFDCreator::createTimerFD(int clockid, int flags)
    {
        return ::timerfd_create(clockid,flags);
    }

    int EpollFDCreator::createEventFD(unsigned int initval, int flags)
    {
        return ::eventfd(initval,flags);
    }

}
