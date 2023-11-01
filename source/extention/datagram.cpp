
#include "datagram.hh"
#include <cmath>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

#include "memclear.hpp"


namespace chrindex::andren::network {

        DataGram::DataGram(base::Socket && sock, std::weak_ptr<RePoller> wrp)
        {
            data = std::make_unique<_private>();
            data->m_sock = std::move(sock);
            data->wrp = wrp;
        }

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

        bool DataGram::bindAddr(sockaddr * addr , size_t size)
        {
            return data->m_sock.valid() && (0==data->m_sock.bind(addr,size));
        }

        bool DataGram::bindAddr(std::string const & domainName)
        {
            sockaddr_un saddr ;
            base::ZeroMemRef(saddr);
            saddr.sun_family = AF_UNIX;
            ::memcpy(saddr.sun_path, domainName.c_str(), 
                std::min(sizeof(saddr.sun_path) -1 , domainName.size()));
            size_t size = domainName.size() + 1 + sizeof(saddr.sun_family);
            return data->m_sock.valid() 
                && bindAddr(reinterpret_cast<sockaddr*>(&saddr), size);
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
            return asend(remote.toAddr(),remote.addrSize(),std::move(message));
        }

        bool DataGram::asend(std::string const & domainName , std::string const& data)
        {
            auto m = data;
            return asend(domainName, std::move(m));
        }

        bool DataGram::asend(std::string const & domainName , std::string && data)
        {
            sockaddr_un saddr ;
            base::ZeroMemRef(saddr);
            saddr.sun_family = AF_UNIX;
            ::memcpy(saddr.sun_path, domainName.c_str(), 
                std::min(sizeof(saddr.sun_path), domainName.size()));
            size_t size = domainName.size() + 1 + sizeof(saddr.sun_family);
            return asend(reinterpret_cast<sockaddr*>(&saddr), size, std::move(data));
        }

        bool DataGram::asend(sockaddr * saddr , size_t saddr_size , std::string const& message)
        {
            auto m = message;
            return asend(saddr, saddr_size, std::move(m));
        }

        bool DataGram::asend(sockaddr * saddr , size_t saddr_size , std::string && message)
        {
            if(!data->m_sock.valid())
            {
                return false;
            }
            if(auto rp = data->wrp.lock(); rp)[[likely]]
            {
                if (auto ev = rp->eventLoopReference().lock(); ev)[[likely]]
                {
                    std::string addr(reinterpret_cast<char const *>(saddr), saddr_size);
                    return ev->addTask([ remote = std::move(addr),  self = shared_from_this(), msg = std::move(message)]()
                    mutable{
                        if(self->data->m_sock.valid())[[likely]]
                        {
                            ssize_t ret = self->data->m_sock.sendto(msg.c_str(), msg.size(), 0, 
                                reinterpret_cast<sockaddr const *>(remote.c_str()),remote.size());
                            if(self->data->m_onWrite){ self->data->m_onWrite(ret,std::move(msg)); }
                        }
                    },TaskDistributorTaskType::IO_TASK);
                }
            }
            return false;
        }

        bool DataGram::aclose(bool unlink_file)
        {
            if (auto rp = data->wrp.lock();rp)[[likely]]
            {
                if(auto ev = rp->eventLoopReference().lock();ev)[[likely]]
                {
                    return ev->addTask([ self = shared_from_this(), unlink_file]()
                    {
                        int fd = self->data->m_sock.handle();
                        int domain = self->data->m_sock.domain();
                        if(domain == AF_UNIX && unlink_file)
                        {
                            self->data->m_sock.unlink();
                        }
                        self->data->m_sock.closeAndNoClear();
                        if(auto rp = self->data->wrp.lock();rp)[[likely]]
                        {
                            // 我手动触发一下。
                            rp->notifyEvents(fd, EPOLLIN);
                        }
                    },TaskDistributorTaskType::IO_TASK);
                }
            }
            return false;
        }

        bool DataGram::startListenReadEvent()
        {
            if(auto rp = data->wrp.lock();rp)[[likely]]
            {
                if(auto ev = rp->eventLoopReference().lock(); ev)[[likely]]
                {
                    int fd = data->m_sock.handle();
                    return ev->addTask([fd ,  self = shared_from_this()]()
                    {
                        self->listenReadEvent(fd);
                    },TaskDistributorTaskType::IO_TASK);
                }
            }
            return false;
        }


        base::KVPair<ssize_t,std::string> DataGram::tryRead(sockaddr_storage & ss, uint32_t &len)
        {
            base::Socket & sock = data->m_sock;
            std::string  tmp(2048,0);
            len = sizeof(ss);
            ssize_t ret = sock.recvfrom(&tmp[0], tmp.size(), MSG_WAITALL ,
                 reinterpret_cast<sockaddr*>(&ss), &len);
            
            if(ret < 0) // no data to read
            {
                return { ret , {} }; //  closed
            }
            else if (ret == 0)[[unlikely]]
            {
                return { ret , {} }; //  closed
            }
            
            tmp.resize(ret);
            return { (ssize_t)ret, std::move(tmp) } ;
        }


        void DataGram::listenReadEvent(int fd)
        {
            if(auto rp = data->wrp.lock(); rp)[[likely]]
            {
                std::weak_ptr<DataGram> wsdatagram = shared_from_this();
                rp->append(fd, EPOLLIN);
                rp->setReadyCallback(fd, [fd, wsdatagram](int events)
                {
                    auto self = wsdatagram.lock();
                    if (!self)[[unlikely]]
                    {
                        return ;
                    }
                    if (events & EPOLLIN)[[likely]]
                    {
                        sockaddr_storage ss ;
                        uint32_t len=0; 
                        base::KVPair<ssize_t , std::string> result = self->tryRead(ss, len);
                        if (result.key() == 0) [[unlikely]]
                        {
                            if(self->data->m_onClose){self->data->m_onClose();} 
                            if(auto rp = self->data->wrp.lock(); rp)
                            {
                                rp->cancle(fd);
                            }
                        }else if(result.key() < 0) [[unlikely]]
                        {
                            if(self->data->m_onClose){self->data->m_onClose();} 
                        }
                        else  [[likely]]
                        {
                            std::string saddr_structure(reinterpret_cast<char const *>(&ss),len);
                            if(self->data->m_onRead){
                                self->data->m_onRead(result.key(),std::move(result.value()),
                                std::move(saddr_structure));
                            }
                        }
                    }
                });
            }
        }


}


