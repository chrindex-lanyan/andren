
#include "tcpstream.hh"

#include "propoller.hh"

namespace chrindex::andren::network
{
    static constexpr int DEFAULT_EVENTS = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLPRI;

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
        if (!pdata || !pdata->m_socket.valid() || !_onConnected)
        {
            return false; 
        }
        auto ev = pdata->m_ev.lock();
        if (!ev)
        {
            return false; 
        }

        return ev->addTask([this,self = shared_from_this(), ip = std::move(_ip) , port ,onConnected = std::move(_onConnected) ]()mutable
        {
            base::EndPointIPV4 epv4(std::move(ip),port);
            //fprintf(stdout,"TcpStream : Preapre Connect %s:%d.\n",ip.c_str(),port);
            if(0 != self->pdata->m_socket.connect(epv4.toAddr(),epv4.addrSize()))
            {
                onConnected(-1);
                return ;
            }
            // 添加read event到poller
            auto pp = self->pdata->m_pp.lock();
            if (pp)
            {
                int events = DEFAULT_EVENTS;
                pp->addEvent(self->pdata->m_socket.handle(), events);
            }
            onConnected(0);
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
        auto clistream = std::make_shared<TcpStream>(std::move(cli));
        auto pp = pdata->m_pp.lock();
        if (pp)
        {
            int events = DEFAULT_EVENTS;
            pp->addEvent(clistream->pdata->m_socket.handle(), events);
        }
        return clistream;
    }

    bool TcpStream::reqRead(OnReadDone onRead, int timeoutMsec)
    {
        if (!pdata || !pdata->m_socket.valid())
        {
            return false;
        }
        auto ev = pdata->m_ev.lock();
        if ( !ev)
        {
            return false;
        }

        ev->addTask([this, self = shared_from_this() , timeoutMsec, onRead = std::move(onRead)]()mutable
        {
            auto ev = pdata->m_ev.lock();
            auto pp = pdata->m_pp.lock();
            if ( !ev || !pp)
            {
                return false;
            }

            int fd = pdata->m_socket.handle();
            int events = DEFAULT_EVENTS;

            pp->findAndWait(fd, events , timeoutMsec, ev.get() , [fd , this , self, 
                _onRead = std::move(onRead)]( ProPollerError errcode ) mutable
            {
                if (errcode == ProPollerError::FD_NOTFOUND_IN_CACHE 
                    || errcode == ProPollerError::FD_FOUND_BUT_EVENT_NOFOUND )
                {
                    _onRead(-2,{});
                    return ;
                }

                auto pp = pdata->m_pp.lock();

                if (errcode == ProPollerError::FD_AND_EVENT_FOUND_BUT_NO_OCCURRED)
                {
                    _onRead(-4,{});
                    return ;
                }

                if (errcode == ProPollerError::OK)
                {
                    base::KVPair<ssize_t , std::string> result ;
                    if (pdata->m_asslio.valid())
                    {
                        result = pdata->m_asslio.read();
                    }
                    else if(pdata->m_socket.valid())
                    {
                        result = tryRead(pdata->m_socket);
                    }

                    if (result.key() ==0)
                    {
                        int fd = handle()->handle();
                        if (pdata->m_onClose)
                        {
                            pdata->m_onClose();  
                        }
                        if(pp)
                        {
                            pp->delEvent(fd);
                        }
                        handle()->close();
                    }else 
                    {
                        _onRead(result.key(),std::move(result.value()));
                    }
                }
                else 
                {
                    _onRead(-5,{});
                }
            } );
        },EventLoopTaskType::IO_TASK);

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
        if (!pdata || !onWrite)
        {
            return false;
        }

        auto evloop = pdata->m_ev.lock();
        auto pp = pdata->m_pp.lock();

        if (!evloop || !pp)
        {
            return false;
        }

        bool bret = evloop->addTask([onWrite = std::move(onWrite),this, self = shared_from_this() , data = std::move(_data)]() mutable
        {
            if (!valid())
            {
                //fprintf(stdout,"TcpStream::reqWrite : Empty Self.\n");
                return ;
            }
            auto ev = pdata->m_ev.lock();
            auto pp = pdata->m_pp.lock();
            if ( !ev || !pp)
            {
                //fprintf(stdout,"TcpStream::reqWrite : Empty ProPoller.\n");
                return ;
            }

            if (!pdata || !pdata->m_socket.valid())
            {
                //fprintf(stdout,"TcpStream::reqWrite : Invalid Socket Object Or Socket fd.\n");
                return ;
            }
            int fd = pdata->m_socket.handle();
            int events = pp->findEvent(fd);

            events = (DEFAULT_EVENTS|EPOLLOUT);
            
            if (!pp->modEvent(fd, events))
            {
                //fprintf(stdout,"TcpStream::reqWrite : Cannot modEvent as RW.\n");
                return ;
            }

            //fprintf(stdout,"TcpStream::reqWrite : PrePare Find And Wait.\n");

            pp->findAndWait(fd, EPOLLOUT , 5000, ev.get() ,
                    [fd ,this , self, _onWrite = std::move(onWrite), _data = std::move(data)](ProPollerError errcode)mutable
            {
                if (errcode == ProPollerError::FD_NOTFOUND_IN_CACHE 
                    || errcode == ProPollerError::FD_FOUND_BUT_EVENT_NOFOUND )
                {
                    _onWrite(-2,std::move(_data));
                    return ;
                }

                auto pp = pdata->m_pp.lock(); 
                if(!pp)
                {
                    //fprintf(stdout,"TcpStream::reqWrite : Empty ProPoller.\n");
                    return ;
                }
                int lastEvent = pp->findEvent(fd);
                lastEvent &= ~(EPOLLOUT);
                if (! pp->modEvent(fd,lastEvent))
                {
                    //fprintf(stdout,"TcpStream::reqWrite : modEvent and Remove Writable Failed.\n");
                    return ;
                }

                if (errcode == ProPollerError::FD_AND_EVENT_FOUND_BUT_NO_OCCURRED)
                {
                    _onWrite(-3,std::move(_data));
                    return  ;
                }

                if (errcode != ProPollerError::OK)
                {
                    //fprintf(stdout,"TcpStream::reqWrite : errcode != OK...Others Error.\n");
                    return ;
                }

                ssize_t ret = 0;
                if (pdata->m_asslio.valid())
                {
                    ret = pdata->m_asslio.write(_data);
                }
                else if(pdata->m_socket.valid())
                {
                    ret = pdata->m_socket.send(_data.c_str(),_data.size(),0);
                }
                else 
                {
                    ret = -4;
                }

                if(_onWrite)
                {
                    _onWrite(ret,std::move(_data));
                }
                return ;
            });
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
        if (valid() && (pdata->m_asslio.valid() == false))
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

    bool TcpStream::setProPoller(std::weak_ptr<ProPoller> wpp)
    {
        auto pp = wpp.lock();
        if (!pdata || !pp)
        {
            return false;
        }
        pdata->m_pp = wpp;
        return true;
    }

    void TcpStream::setEventLoop(std::weak_ptr<EventLoop> ev)
    {
        if (!pdata)
        {
            return ;
        }
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

    void TcpStream::disconnect()
    {
        if(!pdata)
        {
            return ;
        }
        int fd = pdata->m_socket.handle();
        if (fd <= 0)
        {
            return ;
        }
        
        pdata->m_socket.closeAndNoClear();

        bool bret = reqRead([](ssize_t ret, std::string && data)
        {
            fprintf(stdout,"TcpStream::disconnect :  Disconnect.ret=%ld\n",ret);
        }, 5000);

        assert(bret);
    }

}
