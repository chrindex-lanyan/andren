﻿#pragma once

#include "../base/andren_base.hh"


#include "repoller.hh"
#include <memory>
#include <sys/socket.h>


namespace chrindex::andren::network
{

    class DataGram :public std::enable_shared_from_this<DataGram> ,base::noncopyable
    {
    public :

        /// 当可读且读取完成时
        /// 返回ssize_t的值,ret==0时socket关闭，ret>0时有数据，否则无数据或者出错。
        /// 当ret<0时，用户自行判断要不要aclose它。如果aclose，则下一次回调onClose。
        using OnRead  = std::function<void(ssize_t ret , std::string &&, base::EndPointIPV4 remote)>;

        /// 当发送完成时
        using OnWrite = std::function<void(ssize_t ret , std::string &&)>;

        /// 当关闭时
        /// 当socket关闭时。
        using OnClose = std::function<void()>;

        DataGram(std::weak_ptr<RePoller> wrp);
        DataGram(DataGram&&) =delete;
        ~DataGram();

        /// 可以选择为UDP套接字绑定出入口和端口号
        bool bindAddr(std::string const & ip, int port);

        /// 返回内部Socket实例的引用。
        /// 返回值永远不为空。
        base::Socket * reference_handle();

        /// 返回内部repoller弱引用
        std::weak_ptr<RePoller> reference_repoller();

    /// ### Note 这些函数必须在以下情况提前设置好：发送数据前，以及监听可读事件前。

        void setOnClose(OnClose const & cb);

        void setOnRead(OnRead const & cb);

        void setOnWrite(OnWrite const & cb);

        void setOnClose(OnClose && cb);

        void setOnRead(OnRead && cb);

        void setOnWrite(OnWrite && cb);

    /// ###  End Note

        bool asend(base::EndPointIPV4 remote , std::string const & data);

        bool asend(base::EndPointIPV4 remote , std::string && data);

        /// async close。
        /// 如果返回true，则该函数会self = shared_from_this(),
        /// 并将self保存到一个EventLoop任务中。
        bool aclose();

        /// 开始监听ReadEvent。
        /// 调用此函数前，请确保已设置了OnRead、OnWrite、OnClose。
        /// 如果返回true，则该函数会self = shared_from_this(),
        /// 并将self保存到一个EventLoop任务中。
        bool startListenReadEvent();

    private :

        /// 从一个Socket对象读数据(非阻塞读)。
        /// 返回ssize_t的值,ret==0时socket关闭，ret>0时有数据，否则无数据或者出错。
        /// 当ret<0时，用户自行判断要不要aclose它。如果aclose，则下一次回调onClose。
        base::KVPair<ssize_t,std::string> tryRead(sockaddr_storage &ss);

        /// 监听ReadEvent
        void listenReadEvent(int fd);

    private :

        struct _private 
        {
            OnClose m_onClose;
            OnRead m_onRead;
            OnWrite m_onWrite;
            base::Socket m_sock;
            std::weak_ptr<RePoller> wrp;
        };

        std::unique_ptr<_private> data;

    };

}