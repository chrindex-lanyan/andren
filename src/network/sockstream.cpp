#include "sockstream.hh"
#include "eventloop.hh"
#include <memory>

namespace chrindex::andren::network
{
    SockStream::SockStream()
    {
        //
    }

    SockStream::SockStream(base::Socket && sock, std::weak_ptr<RePoller> wrp)
    {
        data = std::make_unique<_private>();

    }
    SockStream::~SockStream()
    {
        //
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
        if(data)
        {
            data->m_onClose = std::move(cb);
        }
    }

    void SockStream::setOnRead(OnRead && cb)
    {
        if(data)
        {
            data->m_onRead = std::move(cb);
        }
    }

    void SockStream::setOnWrite(OnWrite && cb)
    {
        if(data)
        {
            data->m_onWrite = std::move(cb);
        }
    }

    bool SockStream::startListenReadEvent()
    {
        if(!data)
        {
            return false;
        }
        if(auto rp = data->wrp.lock();rp)
        {
            if(auto ev = rp->eventLoopReference().lock(); ev)
            {
                int fd = data->m_sock.handle();
                ev->addTask([fd , this, self = shared_from_this()]()
                {
                    if(!data)
                    {
                        return ;
                    }
                    if(auto rp = data->wrp.lock(); rp)
                    {
                        rp->append(fd, EPOLLIN, 
                            [this,fd, self](int events)
                        {
                            if (events & EPOLLIN)
                            {
                                base::KVPair<ssize_t , std::string> result = tryRead(data->m_sock);
                                if (result.key() <= 0)
                                {
                                    data->m_onClose();
                                    if(auto rp = data->wrp.lock(); rp)
                                    {
                                        rp->cancle(fd, [](int events){});
                                    }
                                }
                                else 
                                {
                                    data->m_onRead(result.key(),std::move(result.value()));
                                }
                            }
                        });
                    }
                },EventLoopTaskType::IO_TASK);
            }
        }
        return true;
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
        if(!data || !data->m_sock.valid())
        {
            return false;
        }
        if(auto rp = data->wrp.lock(); rp)
        {
            if (auto ev = rp->eventLoopReference().lock(); ev)
            {
                return ev->addTask([this, self = shared_from_this(), msg = std::move(_data)]()
                {
                    if(data && data->m_sock.valid())
                    {
                        data->m_sock.send(msg.c_str(), msg.size(), 0);
                    }
                },EventLoopTaskType::IO_TASK);
            }
        }
        return false;
    }

    /// async close
    void SockStream::aclose()
    {
        if (data)
        {
            if (auto rp = data->wrp.lock();rp)
            {
                if(auto ev = rp->eventLoopReference().lock();ev)
                {
                    ev->addTask([this, self = shared_from_this()]()
                    {
                        if(data)
                        {
                            data->m_sock.closeAndNoClear();
                        }
                    },EventLoopTaskType::IO_TASK);
                }
            }
        }
    }

    base::KVPair<ssize_t,std::string> SockStream::tryRead(base::Socket & sock)
    {
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

}

