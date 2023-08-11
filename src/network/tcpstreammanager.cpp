
#include "tcpstreammanager.hh"

namespace chrindex::andren::network
{
    TcpStreamManager::TcpStreamManager()
    {
        m_ep = std::make_shared<base::Epoll>();
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

    TSM_Error TcpStreamManager::start(std::string const &ip, uint32_t port)
    {
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
        m_server.setEpoll(m_ep);
        m_stop = false;
        return TSM_Error::OK;
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
            std::shared_ptr<TcpStream> clistream = self->m_server.accept();
            if(!clistream)
            {
                onAccept( 0 , TSM_Error::ACCEPT_FAILED );
                return false;
            }
            
            onAccept( std::move(clistream) , TSM_Error::OK );
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