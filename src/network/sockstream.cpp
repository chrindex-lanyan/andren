#include "sockstream.hh"
#include "eventloop.hh"
#include <cstdio>
#include <memory>
#include <sys/socket.h>

namespace chrindex::andren::network
{

    SockStream::SockStream(base::Socket && sock, std::weak_ptr<RePoller> wrp)
    {
        data = std::make_unique<_private>();
        data->m_sock = std::move(sock);
        data->wrp = wrp;
        //fprintf(stdout,"SockStream::SockStream %lu.\n",(uint64_t)this);
    }
    SockStream::~SockStream()
    {
        //fprintf(stdout,"SockStream::~SockStream %lu.\n",(uint64_t)this);
    }

    void SockStream::setOnClose(OnClose const & cb)
    {
        auto _cb = cb;
        setOnClose(std::move(_cb));
    }

    void SockStream::setOnRead(OnRead const & cb)
    {
        auto _cb = cb;
        setOnRead(std::move(_cb));
    }

    void SockStream::setOnWrite(OnWrite const & cb)
    {
        auto _cb = cb;
        setOnWrite(std::move(_cb));
    }

    void SockStream::setOnClose(OnClose && cb)
    {
        if(cb)
        {
            data->m_onClose = std::move(cb);
        }
    }

    void SockStream::setOnRead(OnRead && cb)
    {
        if(cb)
        {
            data->m_onRead = std::move(cb);
        }
    }

    void SockStream::setOnWrite(OnWrite && cb)
    {
        if(cb)
        {
            data->m_onWrite = std::move(cb);
        }
    }

    base::Socket * SockStream::reference_handle()
    {
        return &data->m_sock;
    }

    std::weak_ptr<RePoller> SockStream::reference_repoller()
    {
        return data->wrp;
    }

    bool SockStream::startListenReadEvent()
    {
        if(auto rp = data->wrp.lock();rp)
        {
            if(auto ev = rp->eventLoopReference().lock(); ev)
            {
                int fd = data->m_sock.handle();
                return ev->addTask([fd ,  self = shared_from_this()]()
                {
                    self->listenReadEvent(fd);
                },EventLoopTaskType::IO_TASK);
            }
        }
        return false;
    }

    /// async send
    bool SockStream::asend(std::string const & _data)
    {
        auto d = _data;
        return asend(std::move(d));
    }

    /// async send
    bool SockStream::asend(std::string && _data)
    {
        if(!data->m_sock.valid())
        {
            return false;
        }
        if(auto rp = data->wrp.lock(); rp)
        {
            if (auto ev = rp->eventLoopReference().lock(); ev)
            {
                return ev->addTask([ self = shared_from_this(), msg = std::move(_data)]()
                {
                    if(self->data->m_sock.valid())
                    {
                        self->data->m_sock.send(msg.c_str(), msg.size(), 0);
                    }
                },EventLoopTaskType::IO_TASK);
            }
        }
        return false;
    }

    /// async close
    bool SockStream::aclose()
    {
        if (auto rp = data->wrp.lock();rp)
        {
            if(auto ev = rp->eventLoopReference().lock();ev)
            {
                return ev->addTask([ self = shared_from_this()]()
                {
                    self->data->m_sock.closeAndNoClear();
                },EventLoopTaskType::IO_TASK);
            }
        }
        return false;
    }

    bool SockStream::asyncConnect(std::string const &ip , int port, std::function<void(bool bret)> onConnect)
    {
        auto rp = data->wrp.lock();
        auto ev = rp? rp->eventLoopReference().lock() : std::shared_ptr<EventLoop>{};
        if (!ev || !data->m_sock.valid())
        {
            return false;
        }
        base::EndPointIPV4 epv4(ip,port);
        return ev->addTask([ _addr = std::move(epv4) ,cb = std::move(onConnect) ,
             self= shared_from_this()]()mutable
        {
            int ret = self->data->m_sock.connect(_addr.toAddr(), _addr.addrSize());
            if(ret == 0)
            {
                self->listenReadEvent(self->data->m_sock.handle());
            }
            cb(ret == 0);
        },EventLoopTaskType::IO_TASK);
    }

    base::KVPair<ssize_t,std::string> SockStream::tryRead()
    {
        base::Socket & sock = data->m_sock;
        std::string rddata , tmp(128,0);
        bool readok = false;
        while (1)
        {
            ssize_t ret = sock.recv(&tmp[0], tmp.size(), MSG_DONTWAIT);
            if (int errcode = errno; ret < 0 && (errcode == EAGAIN || errcode == EWOULDBLOCK))
            {
                readok = true;  // read done.
                break;
            }
            else if (ret == 0 || ret < 0)
            {
                return { 0 , {} }; // disconnected
            }
            else if (ret > 0)
            {
                rddata.insert(rddata.end(), tmp.begin(), tmp.begin() + ret);
            }
        }
        if (readok)
        {
            return { (ssize_t)rddata.size(), std::move(rddata) } ;
        }
        return { 0, {}};
    }

    void SockStream::listenReadEvent(int fd)
    {
        if(auto rp = data->wrp.lock(); rp)
        {
            std::weak_ptr<SockStream> wsstream = shared_from_this();
            rp->append(fd, EPOLLIN);
            rp->setReadyCallback(fd, [fd, wsstream](int events)
            {
                auto self = wsstream.lock();
                if (!self)
                {
                    return ;
                }
                //fprintf(stdout,"SockStream::listenReadEvent : this = %lu.\n",(uint64_t)self.get());
                if (events & EPOLLIN)
                {
                    base::KVPair<ssize_t , std::string> result = self->tryRead();
                    if (result.key() <= 0)
                    {
                        self->data->m_onClose();
                        if(auto rp = self->data->wrp.lock(); rp)
                        {
                            rp->cancle(fd);
                        }
                    }
                    else 
                    {
                        self->data->m_onRead(result.key(),std::move(result.value()));
                    }
                }
            });
        }
    }

}

