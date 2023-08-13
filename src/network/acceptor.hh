#pragma once

#include "../base/andren_base.hh"
#include <functional>
#include <memory>


#include "eventloop.hh"
#include "sockstream.hh"
#include "repoller.hh"

namespace chrindex::andren::network
{

    class Acceptor : public std::enable_shared_from_this<Acceptor> , base::noncopyable
    {
    public :

        //Acceptor();

        Acceptor(std::weak_ptr<EventLoop> wev , std::weak_ptr<RePoller> wep);
        Acceptor(Acceptor&&)=delete;
        ~Acceptor();

        /// clis为空时socket fd失败，
        /// 否则为已accept的socket。
        using OnAccept = std::function<void(std::shared_ptr<SockStream> clis)>;

        /// 设置accept后的回调
        void setOnAccept(OnAccept const & onAccept);

        /// 设置accept后的回调
        /// 该onAccept是持久化的，请勿保存Acceptor的强引用指针实例，
        /// 以避免循环引用。
        void setOnAccept(OnAccept && onAccept);

        /// 开始监听
        /// 注意，要在POLLER开始后才可以调用这个函数。
        /// 因为这个函数会使用到POLLER内部的事件循环实例。
        bool start(base::Socket && listenSock);

        /// 异步地停止监听
        bool asyncStop(std::function<void()> onStop);

    private :

        struct _private 
        {
            OnAccept m_onAccept;
            std::weak_ptr<EventLoop> m_wev;
            std::weak_ptr<RePoller> m_wep;
            base::Socket m_sock;
        };
        std::unique_ptr<_private> data;

    };


}

