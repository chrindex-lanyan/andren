﻿#pragma once

#include "../../base/andren_base.hh"

#include "../task_distributor.hh"



namespace chrindex::andren::network
{
    
    class UdpPackage : public std::enable_shared_from_this<UdpPackage> , base::noncopyable
    {
    public :
        UdpPackage();
        UdpPackage(UdpPackage && _);
        UdpPackage(base::Socket && sock);
        ~UdpPackage();

        void operator=(UdpPackage && _);

        void setEventLoop(std::weak_ptr<TaskDistributor> ev);

        void setEpoller(std::weak_ptr<base::Epoll> ep);

        bool bind();

        bool start();

        bool valid () const;

        base::Socket * handle();

        bool requestRead();

        bool requestWrite(std::string const & data, base::EndPointIPV4 const & epv4);

        bool requestWrite(std::string && data, base::EndPointIPV4 && epv4);

    private:

        void processEvents();


    private:
        
        struct _Private 
        {
            base::Socket m_sock;
        } ;

        std::unique_ptr<_Private> pdata;
    };




}

