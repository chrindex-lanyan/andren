
#include "tcpstream.hh"

namespace chrindex::andren::network
{
    TcpStream::_Private::_Private()
    {
        m_socket = std::move(base::Socket(AF_INET, SOCK_STREAM, 0));
    }
    TcpStream::_Private::~_Private()
    {
        //
    }

    TcpStream::TcpStream()
    {
        //
    }
    TcpStream::TcpStream(base::Socket &&sock)
    {
        pdata = std::make_unique<_Private>();
        pdata->m_socket = std::move(sock);
    }
    TcpStream::TcpStream(TcpStream &&_)
    {
        pdata = std::move(_.pdata);
    }

    TcpStream::~TcpStream()
    {
        //
    }

    void TcpStream::operator=(TcpStream &&_)
    {
        this->~TcpStream();
        pdata = std::move(_.pdata);
    }

    bool TcpStream::reqConnect(std::string _ip, int32_t port , OnConnected _onConnected )
    {
        auto ev = pdata->m_ev.lock();
        if (!pdata || !pdata->m_socket.valid() || !ev || !_onConnected)
        {
            return false; 
        }
        return ev->addTask([self = shared_from_this(), ip = std::move(_ip) , port ,onConnected = std::move(_onConnected) ]()mutable
        {
            base::EndPointIPV4 epv4(std::move(ip),port);
            if(0 != self->pdata->m_socket.connect(epv4.toAddr(),epv4.addrSize()))
            {
                return ;
            }
            onConnected(self);
            return ;
        }, EventLoopTaskType::IO_TASK);
    }

    std::shared_ptr<TcpStream> TcpStream::accept()
    {
        if (!pdata || !pdata->m_socket.valid())
        {
            return nullptr;
        }
        sockaddr_storage ss;
        uint32_t len;

        base::Socket cli = pdata->m_socket.accept(reinterpret_cast<sockaddr*>(&ss) , &len);
        
        if (!cli.valid())
        {
            return nullptr;
        }
        return std::make_shared<TcpStream>(std::move(cli));
    }

    bool TcpStream::reqRead(OnReadDone onRead, int timeoutMsec)
    {
        if (!pdata || !pdata->m_socket.valid())
        {
            return false;
        }

        auto pp = pdata->m_pp.lock();
        if (!pp)
        {
            return false;
        }
        pp->findAndWait(pdata->m_socket.handle(), [self = shared_from_this(), 
            _onRead = std::move(onRead)](ProEvent proev, bool isTimeout) mutable
        {
            if (isTimeout)
            {
                _onRead(-2,{});
            }
            if (proev.readable)
            {
                base::KVPair<ssize_t , std::string> result ;
                if (self->pdata->m_asslio.valid())
                {
                    result = self->pdata->m_asslio.read();
                }
                else if(self->pdata->m_socket.valid())
                {
                    result = self->tryRead(self->pdata->m_socket);
                }

                if (result.key() ==0)
                {
                    if (self->pdata->m_onClose)
                    {
                        self->pdata->m_onClose();  
                    }
                    auto pp = self->pdata->m_pp.lock();
                    if (pp)
                    {
                        pp->delSubscribe(self->pdata->m_socket.handle());
                    }
                }else 
                {
                    _onRead(result.key(),std::move(result.value()));
                }
            }
        } , timeoutMsec);

        return true;
    }

    bool TcpStream::setOnClose(OnClose onClose)
    {
        if (!pdata)
        {
            return false;
        }
        pdata->m_onClose = std::move(onClose);
        return true;
    }

    bool TcpStream::reqWrite(std::string const &data, OnWriteDone onWrite)
    {
        std::string _data = data;
        return reqWrite(std::move(_data),std::move(onWrite));
    }

    bool TcpStream::reqWrite(std::string && _data, OnWriteDone onWrite)
    {
        if (!pdata )
        {
            return false;
        }

        auto evloop = pdata->m_ev.lock();
        auto pp = pdata->m_pp.lock();

        if (!evloop || !pp)
        {
            return false;
        }

        bool bret = evloop->addTask([onWrite = std::move(onWrite), self = shared_from_this() , data = std::move(_data)]() mutable
        {
            if (!self->valid())
            {
                return ;
            }

            if (auto pp = self->pdata->m_pp.lock(); pp )
            {
                ProEvent proev;
                proev.readable = 1;
                proev.writeable = 1;
                if (pp->modSubscribe(self->pdata->m_socket.handle(), proev))
                {
                    pp->findAndWait(self->pdata->m_socket.handle(),
                        [self, _onWrite = std::move(onWrite), _data = std::move(data)](ProEvent proev, bool timeoutMsec)mutable
                    {
                        ProEvent _proev;
                        _proev.readable = 1;
                        _proev.writeable = 0;
                        auto pp = self->pdata->m_pp.lock(); 
                        if (!pp || !pp->modSubscribe(self->pdata->m_socket.handle(), _proev))
                        {
                            return ;
                        }
                        if (timeoutMsec)
                        {
                            _onWrite(-2,_data);
                            return ;
                        }
                        if (proev.writeable)
                        {
                            ssize_t ret = 0;
                            if (self->pdata->m_asslio.valid())
                            {
                                ret = self->pdata->m_asslio.write(_data);
                            }
                            else if(self->pdata->m_socket.valid())
                            {
                                ret = self->pdata->m_socket.send(_data.c_str(),_data.size(),0);
                            }
                            else 
                            {
                                ret = -3;
                            }
                            if(_onWrite)
                            {
                                _onWrite(ret,std::move(_data));
                            }
                        }
                    },5000);
                }
            }
        }
        ,EventLoopTaskType::IO_TASK);

        return bret;
    }

    base::Socket *TcpStream::handle()
    {
        return pdata ? (&pdata->m_socket) : (nullptr);
    }

    bool TcpStream::valid() const
    {
        return pdata != nullptr && pdata->m_socket.valid();
    }

    /// ######

    bool TcpStream::enableSSL(base::aSSL && assl)
    {
        if (valid() && pdata->m_asslio.valid() == false)
        {
            pdata->m_asslio.upgradeFromSSL(std::move(assl));
            return 1 == pdata->m_asslio.bindSocketFD(pdata->m_socket.handle());
        }
        return false;
    }

    bool TcpStream::usingSSL() const
    {
        return valid() && pdata->m_asslio.valid();
    }

    base::aSSLSocketIO *TcpStream::sslioHandle()
    {
        if (valid() && pdata->m_asslio.valid())
        {
            return &pdata->m_asslio;
        }
        return nullptr;
    }

    /// ######

    void TcpStream::setProPoller(std::weak_ptr<ProPoller> wpp)
    {
        auto pp = wpp.lock();
        if (!pp)
        {
            return ;
        }
        pdata->m_pp = wpp;
        ProEvent proev;
        proev.readable = 1;
        pp->subscribe(pdata->m_socket.handle(), proev);
    }

    void TcpStream::setEventLoop(std::weak_ptr<EventLoop> ev)
    {
        pdata->m_ev = ev;
    }

    base::KVPair<ssize_t,std::string> TcpStream::tryRead(base::Socket & sock)
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
