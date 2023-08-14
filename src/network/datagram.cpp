
#include "datagram.hh"
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>



namespace chrindex::andren::network {

        DataGram::DataGram(std::weak_ptr<RePoller> wrp)
        {
            data = std::make_unique<_private>();
            data->m_sock = base::Socket(AF_INET,SOCK_DGRAM,0);
            data->wrp = wrp;
        }


        DataGram::~DataGram()
        {

        }


        bool DataGram::bindAddr(std::string const & ip, int port)
        {
            base::EndPointIPV4 epv4(ip,port);
            return data->m_sock.valid() && (0==data->m_sock.bind(epv4.toAddr(), epv4.addrSize()));
        }


        base::Socket * DataGram::reference_handle()
        {
            return &data->m_sock;
        }

        std::weak_ptr<RePoller> DataGram::reference_repoller()
        {
            return data->wrp;
        }

        void DataGram::setOnClose(OnClose const & cb)
        {
            auto _cb = cb;
            setOnClose(std::move(_cb));
        }

        void DataGram::setOnRead(OnRead const & cb)
        {
            auto _cb = cb;
            setOnRead(std::move(_cb));
        }

        void DataGram::setOnWrite(OnWrite const & cb)
        {
            auto _cb = cb;
            setOnWrite(std::move(_cb));
        }

        void DataGram::setOnClose(OnClose && cb)
        {
            data->m_onClose = std::move(cb);
        }

        void DataGram::setOnRead(OnRead && cb)
        {
            data->m_onRead = std::move(cb);
        }

        void DataGram::setOnWrite(OnWrite && cb)
        {
            data->m_onWrite = std::move(cb);
        }


        bool DataGram::asend(base::EndPointIPV4 remote , std::string const & data)
        {
            auto d = data;
            return asend(std::move(remote), std::move(d));
        }

        bool DataGram::asend(base::EndPointIPV4 remote , std::string && message)
        {
            if(!data->m_sock.valid())
            {
                return false;
            }
            if(auto rp = data->wrp.lock(); rp)
            {
                if (auto ev = rp->eventLoopReference().lock(); ev)
                {
                    return ev->addTask([ remote = std::move(remote),  self = shared_from_this(), msg = std::move(message)]()
                    mutable{
                        if(self->data->m_sock.valid())
                        {
                            ssize_t ret = self->data->m_sock.sendto(msg.c_str(), msg.size(), 0, remote.toAddr(),remote.addrSize());
                            if(self->data->m_onWrite){ self->data->m_onWrite(ret,std::move(msg)); }
                        }
                    },EventLoopTaskType::IO_TASK);
                }
            }
            return false;
        }

        bool DataGram::aclose()
        {
            if (auto rp = data->wrp.lock();rp)
            {
                if(auto ev = rp->eventLoopReference().lock();ev)
                {
                    return ev->addTask([ self = shared_from_this()]()
                    {
                        int fd = self->data->m_sock.handle();
                        //fprintf(stdout,"DataGram::aclose ::FD [%d] Close And No Clear.\n",fd);
                        self->data->m_sock.closeAndNoClear();
                        if(auto rp = self->data->wrp.lock();rp)
                        {
                            // UDP Socket的Closed似乎不会触发EPOLLIN，所以我手动触发一下。
                            rp->notifyEvents(fd, EPOLLIN);
                        }
                    },EventLoopTaskType::IO_TASK);
                }
            }
            return false;
        }

        bool DataGram::startListenReadEvent()
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


        base::KVPair<ssize_t,std::string> DataGram::tryRead(sockaddr_storage & ss)
        {
            base::Socket & sock = data->m_sock;
            std::string  tmp(2048,0);
            uint32_t len= sizeof(ss);
            ssize_t ret = sock.recvfrom(&tmp[0], tmp.size(), MSG_WAITALL , reinterpret_cast<sockaddr*>(&ss), &len);
            
            if(ret < 0) // no data to read
            {
                return { ret , {} }; // remote closed
            }
            else if (ret == 0)
            {
                return { ret , {} }; // remote closed
            }
            
            tmp.resize(ret);
            return { (ssize_t)ret, std::move(tmp) } ;
        }


        void DataGram::listenReadEvent(int fd)
        {
            if(auto rp = data->wrp.lock(); rp)
            {
                std::weak_ptr<DataGram> wsdatagram = shared_from_this();
                rp->append(fd, EPOLLIN);
                rp->setReadyCallback(fd, [fd, wsdatagram](int events)
                {
                    auto self = wsdatagram.lock();
                    if (!self)
                    {
                        return ;
                    }
                    if (events & EPOLLIN)
                    {
                        sockaddr_storage ss;
                        base::KVPair<ssize_t , std::string> result = self->tryRead(ss);
                        if (result.key() == 0)
                        {
                            if(self->data->m_onClose){self->data->m_onClose();} 
                            if(auto rp = self->data->wrp.lock(); rp)
                            {
                                rp->cancle(fd);
                            }
                        }else if(result.key() < 0)
                        {
                            if(self->data->m_onClose){self->data->m_onClose();} 
                        }
                        else 
                        {
                            base::EndPointIPV4 epv4(reinterpret_cast<sockaddr_in*>(&ss));
                            if(self->data->m_onRead){self->data->m_onRead(result.key(),std::move(result.value()),std::move(epv4));}
                        }
                    }
                });
            }
        }


}


