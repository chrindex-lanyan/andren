
#include "tcpstreammanager.hh"

#include <sys/socket.h>

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

    void TcpStreamManager::setEventLoop(std::weak_ptr<TaskDistributor> ev)
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
        m_server.setProPoller(pp);
        m_stop = false;

        auto pev = m_ev.lock();
        if (!pev)
        {
            return TSM_Error::EVENTLOOP_ADD_TASK_FAILED;
        }

        return  (TSM_Error::OK);
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
        int fd = m_server.handle()->handle();
        bool bret = pev->addTask([fd , self = shared_from_this(), onAccept = std::move(onAccept)]()
        {
            if(self->m_stop) // exit
            {
                onAccept(0,TSM_Error::STOPPED);
                return ;
            }
            auto pp = self->m_pp.lock();
            if (!pp || !self->m_server.valid())
            {
                onAccept(0,TSM_Error::SERVER_SOCKET_INVALID);
                return ;
            }
            auto pev = self->m_ev.lock();
            if (!pev )
            {
                onAccept(0,TSM_Error::POLLER_FAILED);
                return ;
            }

            int events = pp->findEvent(fd);
            
            events = (events == -1)?(EPOLLIN):(events|EPOLLIN);
            pp->modEvent(fd, events);

            pp->findAndWait(fd, EPOLLIN, 5000 , pev.get() ,
            [fd , self, _onAccept = std::move(onAccept)](ProPollerError errcode) mutable
            {
                if (errcode == ProPollerError::FD_NOTFOUND_IN_CACHE 
                    || errcode == ProPollerError::FD_FOUND_BUT_EVENT_NOFOUND )
                {
                    _onAccept(0, TSM_Error::POLLER_A_UNKNOW_EVENT );
                    return ;
                }

                auto pp = self->m_pp.lock();
                if (!pp || !self->m_server.valid())
                {
                    _onAccept(0, TSM_Error::POLLER_FAILED );
                    return ;
                }

                int events = pp->findEvent(fd);

                events &= ~(EPOLLIN);
                pp->modEvent(fd, events);

                if (errcode == ProPollerError::FD_AND_EVENT_FOUND_BUT_NO_OCCURRED)
                {
                    _onAccept(0, TSM_Error::ACCEPT_TIMEOUT );
                    return  ;
                }

                if (errcode == ProPollerError::OK)
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
            } );
            
            return ;
        } , 
        TaskDistributorTaskType::IO_TASK);

        return bret ? (TSM_Error::OK) : (TSM_Error::EVENTLOOP_ADD_TASK_FAILED);
    }

    void TcpStreamManager::stop()
    {
        m_stop=true;
    }

}