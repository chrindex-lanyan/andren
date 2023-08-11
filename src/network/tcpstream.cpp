
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

    bool TcpStream::connect(std::string ip, int32_t port, bool aSync)
    {
        if (!pdata || !pdata->m_socket.valid())
        {
            return false;
        }

        base::EndPointIPV4 epv4(std::move(ip),port);
        if(0 != pdata->m_socket.connect(epv4.toAddr(),epv4.addrSize()))
        {
            return false;
        }
        if (pdata->m_asslio.valid()) // 检查并尝试使用SSL
        {
            int ret = 0;
            if (int endType = pdata->m_asslio.endType(); endType ==1)
            {
                ret = pdata->m_asslio.handShake(); // 服务端
            }else if(endType==2)
            {
                ret = pdata->m_asslio.initiateHandShake(); // 客户端
            }

            if (ret!=1)
            {
                return false;
            }
        }
        return true;
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

    bool TcpStream::reqRead(OnReadDone onRead)
    {
        if (!pdata)
        {
            return false;
        }

        auto evloop = pdata->m_ev.lock();
        auto epoller = pdata->m_ep.lock();

        if (!evloop || !epoller)
        {
            return false;
        }

        bool bret = evloop->addTask([onRead = std::move(onRead), self = shared_from_this()]() mutable
        {
            if (!self->valid())
            {
                return ;
            }
            self->pdata->m_onRead = std::move(onRead);
        }
        ,EventLoopTaskType::IO_TASK);

        return bret;
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

    bool TcpStream::reqWrite(std::string &&data, OnWriteDone onWrite)
    {
        if (!pdata)
        {
            return false;
        }

        auto evloop = pdata->m_ev.lock();
        auto epoller = pdata->m_ep.lock();

        if (!evloop || !epoller)
        {
            return false;
        }

        bool bret = evloop->addTask([onWrite = std::move(onWrite), self = shared_from_this() , data = std::move(data)]() mutable
        {
            if (!self->valid())
            {
                return ;
            }
            self->pdata->m_onWrite = std::move(onWrite);
            self->pdata->m_wrbuffer = std::move(data);

            if (auto epoller = self->pdata->m_ep.lock(); epoller )
            {
                epoll_event ev;
                ev.data.u64 = 0;
                ev.events = EPOLLIN | EPOLLOUT;
                epoller->control(base::EpollCTRL::MOD, self->pdata->m_socket.handle(),ev);
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

    void TcpStream::setEpoll(std::weak_ptr<base::Epoll> ep)
    {
        pdata->m_ep = ep;
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

    bool TcpStream::processEvents(int events)
    {
        if (!valid())
        {
            return false;
        }
        
        if (int ret = events;ret == static_cast<int>(SocketStreamEvents::CLOSED))
        {
            return false;
        }
        else if ( ret == static_cast<int>(SocketStreamEvents::READABLE))
        {
            std::string rdb ;
            OnReadDone onRead = std::move(pdata->m_onRead);
            base::KVPair<ssize_t, std::string> result ;

            if (pdata->m_asslio.valid())
            {
                result = pdata->m_asslio.read();
            }
            else if(pdata->m_socket.valid())
            {
                result = tryRead(pdata->m_socket);
            }

            if (result.key() <= 0)  // disconnected
            {
                if (auto epoller = pdata->m_ep.lock(); epoller )
                {
                    epoll_event ev;
                    ev.data.u64 = 0;
                    ev.events = EPOLLIN | EPOLLOUT;
                    epoller->control(base::EpollCTRL::DEL, pdata->m_socket.handle(),ev);
                }

                if (pdata->m_onClose)
                {
                    pdata->m_onClose(); 
                }
            }

            if (onRead) // read done
            {
                onRead(result.key(),result.value()); 
            }
        }
        else if(ret & static_cast<int>(SocketStreamEvents::WRITABLE))
        {
            if (auto epoller = pdata->m_ep.lock(); epoller ) // 去除可写通知
            {
                epoll_event ev;
                ev.data.u64 = 0;
                ev.events = EPOLLIN;
                epoller->control(base::EpollCTRL::MOD, pdata->m_socket.handle(),ev);
            }

            std::string wdb = std::move(pdata->m_wrbuffer) ;
            OnWriteDone onWriteDone = std::move(pdata->m_onWrite);
            ssize_t ret = 0;
            if (pdata->m_asslio.valid())
            {
                ret = pdata->m_asslio.write(wdb);
            }
            else if(pdata->m_socket.valid())
            {
                ret = pdata->m_socket.send(wdb.c_str(),wdb.size(),0);
            }
            else 
            {
                return false;
            }

            if(onWriteDone)
            {
                onWriteDone(ret,wdb);
            }
        }
        return true;
    }
    
    int TcpStream::notify(bool bread, bool bwrite, bool bexcept)
    {
        int ret = 0;

        if (bread)
        {
            ret |= static_cast<int>(SocketStreamEvents::READABLE);
        }
        if(bwrite)
        {
            ret |= static_cast<int>(SocketStreamEvents::WRITABLE);
        }
        if (bexcept)
        {
            ret |= static_cast<int>(SocketStreamEvents::CLOSED);
        }

        return processEvents(ret) ? 0 : -1;
    }

}
