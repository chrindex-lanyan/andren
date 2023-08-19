#include "sslstream.hh"
#include "eventloop.hh"
#include <chrono>
#include <cstdio>
#include <memory>
#include <sys/socket.h>
#include <thread>

namespace chrindex::andren::network
{

    SSLStream::SSLStream(base::Socket && sock, std::weak_ptr<RePoller> wrp)
    {
        data = std::make_unique<_private>();
        data->m_sock = std::move(sock);
        data->wrp = wrp;
        data->m_usedSSL = false;
        data->m_sslshutdown = false;
        //fprintf(stdout,"SSLStream::SSLStream %lu.\n",(uint64_t)this);
    }
    SSLStream::~SSLStream()
    {
        //fprintf(stdout,"SSLStream::~SSLStream %lu.\n",(uint64_t)this);
    }

    bool SSLStream::usingSSL(base::aSSL && assl, int endType)
    {
        data->m_sslio.upgradeFromSSL(std::move(assl));
        data->m_sslio.bindSocketFD(data->m_sock.handle());
        data->m_sslio.setEndType(endType);
        data->m_usedSSL = true;
        return true;
    }

    bool SSLStream::enabledSSL() const 
    {
        return data->m_usedSSL;
    }

    void SSLStream::setOnClose(OnClose const & cb)
    {
        auto _cb = cb;
        setOnClose(std::move(_cb));
    }

    void SSLStream::setOnRead(OnRead const & cb)
    {
        auto _cb = cb;
        setOnRead(std::move(_cb));
    }

    void SSLStream::setOnWrite(OnWrite const & cb)
    {
        auto _cb = cb;
        setOnWrite(std::move(_cb));
    }

    void SSLStream::setOnClose(OnClose && cb)
    {
        if(cb)
        {
            data->m_onClose = std::move(cb);
        }
    }

    void SSLStream::setOnRead(OnRead && cb)
    {
        if(cb)
        {
            data->m_onRead = std::move(cb);
        }
    }

    void SSLStream::setOnWrite(OnWrite && cb)
    {
        if(cb)
        {
            data->m_onWrite = std::move(cb);
        }
    }

    base::Socket * SSLStream::reference_handle()
    {
        return &data->m_sock;
    }

    std::weak_ptr<RePoller> SSLStream::reference_repoller()
    {
        return data->wrp;
    }

    base::aSSLSocketIO * SSLStream::reference_sslio()
    {
        if(!data->m_usedSSL && data->m_sslio.valid())
        {
           return &data->m_sslio; 
        }
        return nullptr;
    }

    bool SSLStream::startListenReadEvent()
    {
        if(auto rp = data->wrp.lock();rp)[[likely]]
        {
            if(auto ev = rp->eventLoopReference().lock(); ev)[[likely]]
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
    bool SSLStream::asend(std::string const & _data)
    {
        auto d = _data;
        return asend(std::move(d));
    }

    /// async send
    bool SSLStream::asend(std::string && _data)
    {
        if(!data->m_sock.valid())
        {
            return false;
        }
        if(auto rp = data->wrp.lock(); rp)[[likely]]
        {
            if (auto ev = rp->eventLoopReference().lock(); ev)[[likely]]
            {
                return ev->addTask([ self = shared_from_this(), msg = std::move(_data)]()
                mutable{
                    if(self->data->m_sock.valid())
                    {
                        ssize_t ret = self->real_write(msg.c_str(), msg.size(), 0);
                        if (self->data->m_onWrite){ self->data->m_onWrite( ret, std::move(msg)); }
                    }
                },EventLoopTaskType::IO_TASK);
            }
        }
        return false;
    }

    /// async close
    bool SSLStream::aclose()
    {
        if (auto rp = data->wrp.lock();rp)[[likely]]
        {
            if(auto ev = rp->eventLoopReference().lock();ev)[[likely]]
            {
                return ev->addTask([ self = shared_from_this()]()
                {
                    self->real_close();
                },EventLoopTaskType::IO_TASK);
            }
        }
        return false;
    }

    bool SSLStream::asyncConnect(std::string const &ip , int port, std::function<void(bool bret)> onConnect)
    {
        auto rp = data->wrp.lock();
        auto ev = rp? rp->eventLoopReference().lock() : std::shared_ptr<EventLoop>{};
        if (!ev || !data->m_sock.valid())[[unlikely]]
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
            if (cb){ cb(ret == 0); }
        },EventLoopTaskType::IO_TASK);
    }

    base::KVPair<ssize_t,std::string> SSLStream::tryRead()
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
            else if (ret == 0 || ret < 0)[[unlikely]]
            {
                return { 0 , {} }; // disconnected
            }
            else if (ret > 0)[[likely]]
            {
                rddata.insert(rddata.end(), tmp.begin(), tmp.begin() + ret);
            }
        }
        if (readok)[[likely]]
        {
            return { (ssize_t)rddata.size(), std::move(rddata) } ;
        }
        return { 0, {}};
    }

    void SSLStream::listenReadEvent(int fd)
    {
        if(auto rp = data->wrp.lock(); rp)
        {
            std::weak_ptr<SSLStream> wsstream = shared_from_this();
            rp->append(fd, EPOLLIN);
            rp->setReadyCallback(fd, [fd, wsstream](int events)
            {
                auto self = wsstream.lock();
                if (!self)[[unlikely]]
                {
                    return ;
                }
                //fprintf(stdout,"SSLStream::listenReadEvent : this = %lu.\n",(uint64_t)self.get());
                if (events & EPOLLIN)
                {
                    base::KVPair<ssize_t , std::string> result = self->real_read();
                    if (result.key() <= 0)[[unlikely]]
                    {
                        if(self->data->m_onClose){self->data->m_onClose();}
                        if(auto rp = self->data->wrp.lock(); rp)
                        {
                            rp->cancle(fd);
                        }
                    }
                    else 
                    {
                        if(self->data->m_onRead){self->data->m_onRead(result.key(),std::move(result.value())); }
                    }
                }
            });
        }
    }

    base::KVPair<ssize_t , std::string> SSLStream::real_read()
    {
        if(data->m_sslshutdown)
        {
            return {-1,{}};
        }
        if (!data->m_usedSSL)
        {
            return tryRead();
        }
        if (!data->m_sslio.valid())
        {
            return {-2, {}};
        }
        return data->m_sslio.read();
    }

    ssize_t SSLStream::real_write(char const * msg , size_t size , int flags)
    {
        if(data->m_sslshutdown)
        {
            return -1;
        }
        if (!data->m_usedSSL)
        {
            return data->m_sock.send(msg, size, flags);
        }
        if (!data->m_sslio.valid())
        {
            return -2;
        }
        return data->m_sslio.write(msg,size); 
    }

    void SSLStream::real_close()
    {
        if (!data->m_usedSSL)
        {
            data->m_sock.closeAndNoClear();
            return ;
        }
        if (!data->m_sslio.valid())
        {
            data->m_sock.closeAndNoClear();
            return ;
        }
        while(1)
        {
            if (int ret = data->m_sslio.shutdown() ; ret == 1 || ret < 0)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        data->m_sslshutdown = true;
        data->m_sock.closeAndNoClear();
    }

}

