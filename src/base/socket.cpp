
#include "socket.hh"

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
#include <sys/ioctl.h>
#include <netdb.h>

#include <stdarg.h>

#include <errno.h>
#include <assert.h>

namespace chrindex::andren::base
{

    Socket::Socket() { m_fd = 0; }
    Socket::~Socket()
    {
        if (m_fd > 0)
        {
        }
        ::close(m_fd);
    }

    Socket::Socket(int fd) { m_fd = fd; }

    Socket::Socket(int domain, int type, int protocol)
    {
        m_fd = 0;
        int fd = ::socket(domain, type, protocol);
        if (fd > 0)
        {
            m_fd = fd;
        }
    }

    Socket::Socket(Socket &&_)
    {
        m_fd = _.m_fd;
        _.m_fd = 0;
    }

    Socket &Socket::operator=(Socket &&_)
    {
        this->~Socket();
        m_fd = _.m_fd;
        _.m_fd = 0;
        return *this;
    }

    bool Socket::valid() const { return m_fd > 0; }

    int Socket::managedFD(int fd)
    {
        if (m_fd > 0)
        {
            return -1;
        }
        m_fd = fd;
        return 0;
    }

    int Socket::bind(struct sockaddr const *addr, uint32_t addrlen)
    {
        return ::bind(m_fd, addr, addrlen);
    }

    int Socket::accept(struct sockaddr *addr, uint32_t *addrlen)
    {
        return ::accept(m_fd, addr, addrlen);
    }

    int Socket::accept4(struct sockaddr *addr, uint32_t *addrlen, int flags)
    {
        return ::accept4(m_fd, addr, addrlen, flags);
    }

    int Socket::connect(struct sockaddr const *addr, uint32_t addrlen)
    {
        return ::connect(m_fd, addr, addrlen);
    }

    int Socket::fcntl(int cmd)
    {
        return ::fcntl(m_fd, cmd);
    }

    int Socket::fcntl(int cmd, long arg)
    {
        return ::fcntl(m_fd, cmd, arg);
    }

    int Socket::getpeername(struct sockaddr *addr, uint32_t *addrlen)
    {
        return ::getpeername(m_fd, addr, addrlen);
    }

    int Socket::getsockname(struct sockaddr *addr, uint32_t *addrlen)
    {
        return ::getsockname(m_fd, addr, addrlen);
    }

    int Socket::getsockopt(int level, int optname, void *optval, uint32_t *optlen)
    {
        return ::getsockopt(m_fd, level, optname, optval, optlen);
    }

    int Socket::setsockopt(int level, int optname, void const *optval, uint32_t optlen)
    {
        return ::setsockopt(m_fd, level, optname, optval, optlen);
    }

    int Socket::ioctl(unsigned long request, ...)
    {
        va_list va;
        va_start(va, &request);
        int ret = ::ioctl(m_fd, request, va);
        va_end(va);
        return ret;
    }

    int Socket::listen(int backlog)
    {
        return ::listen(m_fd, backlog);
    }

    ssize_t Socket::recv(void *buf, size_t len, int flags)
    {
        return ::recv(m_fd, buf, len, flags);
    }

    ssize_t Socket::recvfrom(void *buf, size_t len, int flags,
                             struct sockaddr *src_addr,
                             uint32_t *addrlen)
    {
        return ::recvfrom(m_fd, buf, len, flags, src_addr, addrlen);
    }

    ssize_t Socket::recvmsg(struct msghdr *msg, int flags)
    {
        return ::recvmsg(m_fd, msg, flags);
    }

    int Socket::recvmmsg(struct mmsghdr *msgvec, unsigned int vlen,
                         int flags, struct timespec *timeout)
    {
        return ::recvmmsg(m_fd, msgvec, vlen, flags, timeout);
    }

    ssize_t Socket::send(void const *buf, size_t len, int flags)
    {
        return ::send(m_fd, buf, len, flags);
    }

    ssize_t Socket::sendto(void const *buf, size_t len, int flags,
                           struct sockaddr const *dest_addr, uint32_t addrlen)
    {
        return ::sendto(m_fd, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t Socket::sendmsg(struct msghdr const *msg, int flags)
    {
        return ::sendmsg(m_fd, msg, flags);
    }

    int Socket::sendmmsg(struct mmsghdr *msgvec, unsigned int vlen,
                         int flags)
    {
        return ::sendmmsg(m_fd, msgvec, vlen, flags);
    }

    int Socket::shutdown(int how)
    {
        return ::shutdown(m_fd, how);
    }

    int Socket::handle()const
    {
        return m_fd;
    }

    void Socket::close()
    {
        ::close(m_fd);
        m_fd = 0;
    }

    LocalSocket::LocalSocket()
    {
        m_sv[0] = -1;
        m_sv[1] = -1;
    }
    LocalSocket::LocalSocket(int domain, int type, int protocol)
    {
        int ret = ::socketpair(domain, type, protocol, m_sv);
        if (ret != 0)
        {
            m_sv[0] = -1;
            m_sv[1] = -1;
        }
    }
    LocalSocket::~LocalSocket()
    {
        if (m_sv[0] > 0)
        {
            ::close(m_sv[0]);
        }
        if (m_sv[1] > 0)
        {
            ::close(m_sv[1]);
        }
    }

    bool LocalSocket::asSocketInstance(Socket &first, Socket &second)
    {
        if (m_sv[0] > 0 && m_sv[1] > 0 && first.valid() == false && second.valid() == false)
        {
            first.managedFD(m_sv[0]);
            second.managedFD(m_sv[1]);
            m_sv[0] = -1;
            m_sv[1] = -1;
            return true;
        }
        return false;
    }

    int SelectOperator::select(int nfds, fd_set *readfds,
                               fd_set *writefds,
                               fd_set *exceptfds,
                               struct timeval *timeout)
    {
        return ::select(nfds, readfds, writefds, exceptfds, timeout);
    }

    void SelectOperator::fdClear(int fd, fd_set *set)
    {
        FD_CLR(fd, set);
    }

    int SelectOperator::fdInSet(int fd, fd_set *set)
    {
        return FD_ISSET(fd, set);
    }

    void SelectOperator::fdSet(int fd, fd_set *set)
    {
        FD_SET(fd, set);
    }

    void SelectOperator::fdZero(fd_set *set)
    {
        FD_ZERO(set);
    }

    int SelectOperator::pselect(int nfds, fd_set *readfds,
                                fd_set *writefds,
                                fd_set *exceptfds,
                                const struct timespec *timeout,
                                const sigset_t *sigmask)
    {
        return ::pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
    }

}
