
#include "tcpstreammanager.hh"

namespace chrindex::andren::network
{
    TcpStreamManager::TcpStreamManager()
    {
        m_stop = true;
    }

    TcpStreamManager::~TcpStreamManager()
    {
        stop();
    }

    void TcpStreamManager::setEventLoop(std::weak_ptr<EventLoop> ev)
    {
        m_ev = std::move(ev);
    }


    void TcpStreamManager::setProPoller(std::weak_ptr<ProPoller> pp)
    {
        m_pp = pp;
    }

    TSM_Error TcpStreamManager::start(std::string const &ip, uint32_t port)
    {
        auto pp = m_pp.lock();
        if (!pp)
        {
            return TSM_Error::POLLER_FAILED;
        }

        base::Socket sock(AF_INET, SOCK_STREAM, 0);
        base::EndPointIPV4 epv4(ip,port);

        int optval = 1;
        if(0!=sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
        {
            return TSM_Error::SET_SOCKET_OPT_FAILED;
        }
        if(0!=sock.bind(epv4.toAddr(),epv4.addrSize()))
        {
            return TSM_Error::BIND_ADDR_FAILED;
        }
        if(0!=sock.listen(100))
        {
            return TSM_Error::LISTEN_SOCKET_FAILED;
        }
        m_server = std::move(sock);
        //m_server.setEpoll(m_ep);
        m_server.setProPoller(pp);
        m_stop = false;

        auto pev = m_ev.lock();
        if (!pev)
        {
            return TSM_Error::EVENTLOOP_ADD_TASK_FAILED;
        }
        bool bret = pev->addTask([self = shared_from_this()]()
        {
            auto pp = self->m_pp.lock();
            if (!pp || !self->m_server.valid())
            {
                return ;
            }
            ProEvent proEvent;
            proEvent.readable = 1;
            bool bret = pp->subscribe(self->m_server.handle()->handle(), proEvent);
        },EventLoopTaskType::IO_TASK);

        return bret ? (TSM_Error::OK):(TSM_Error::EVENTLOOP_ADD_TASK_FAILED);
    }

    TSM_Error TcpStreamManager::requestAccept(OnAccept onAccept)
    {
        auto pev = m_ev.lock();
        if (!pev || !m_server.valid())
        {
            return TSM_Error::SERVER_SOCKET_INVALID;
        }
        if (!onAccept)
        {
            return TSM_Error::ACCEPT_CALLBACK_EMPTY;
        }
        bool bret = pev->addTask([self = shared_from_this(), onAccept = std::move(onAccept)]()->bool
        {
            if(self->m_stop) // exit
            {
                onAccept(0,TSM_Error::STOPPED);
                return false;
            }
            auto pp = self->m_pp.lock();
            if (!pp || !self->m_server.valid())
            {
                return false;
            }
            pp->findAndWait(self->m_server.handle()->handle(),
            [self, _onAccept = std::move(onAccept)](ProEvent proev, bool isTimeout) mutable
            {
                if (isTimeout)
                {
                    self->requestAccept(std::move(_onAccept));
                    return ;
                }

                if (proev.readable)
                {
                    std::shared_ptr<TcpStream> clistream = self->m_server.accept();
                    if(!clistream)
                    {
                        _onAccept( 0 , TSM_Error::ACCEPT_FAILED );
                        return ;
                    }
                    clistream->setEventLoop(self->m_ev);
                    clistream->setProPoller(self->m_pp);
                    _onAccept( std::move(clistream) , TSM_Error::OK );
                }
                else 
                {
                    _onAccept( 0 , TSM_Error::ACCEPT_FAILED );
                }
            }, 50 );
            
            return true;
        } , 
        EventLoopTaskType::IO_TASK);

        return bret ? (TSM_Error::OK) : (TSM_Error::EVENTLOOP_ADD_TASK_FAILED);
    }

    void TcpStreamManager::stop()
    {
        m_stop=true;
    }

}