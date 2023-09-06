#pragma once

#include "../base/andren_base.hh"

#include "repoller.hh"

#include <functional>
#include <memory>
#include <sys/socket.h>


namespace chrindex::andren::network
{

    class SockStream: public std::enable_shared_from_this<SockStream> , base::noncopyable
    {
    public :

        /// 当可读且读取完成时
        using OnRead  = std::function<void(ssize_t ret , std::string &&)>;

        /// 当发送完成时
        using OnWrite = std::function<void(ssize_t ret , std::string &&)>;

        /// 当关闭时
        using OnClose = std::function<void()>;

        SockStream(base::Socket && sock , std::weak_ptr<RePoller> wrp);
        SockStream(SockStream&&) = delete;
        ~SockStream();

     ///### Note 这些函数必须在以下情况提前设置好：Acceptor的OnAccept回调时、SockStream的asyncConnect前。

        void setOnClose(OnClose const & cb);

        void setOnRead(OnRead const & cb);

        void setOnWrite(OnWrite const & cb);

        void setOnClose(OnClose && cb);

        void setOnRead(OnRead && cb);

        void setOnWrite(OnWrite && cb);

    /// ###  End Note

        /// 开始监听ReadEvent。
        /// 如果通过asyncConnect连接，则无需调用此函数；
        /// 如果通过Acceptor创建此SockStream实例，则需要调用该函数；
        /// 如果通过自定Socket实例创建本实例，则取决于本实例创建前，
        /// 是否已经为自定Socket设置Epoll事件监听。
        /// 调用此函数前，请确保已设置了OnRead、OnWrite、OnClose。
        /// 如果返回true，则该函数会self = shared_from_this(),
        /// 并将self保存到一个EventLoop任务中。
        bool startListenReadEvent();

        /// async send。
        /// 该函数调用`bool asend(std::string && _data)`函数。
        bool asend(std::string const & _data);

        /// async send。
        /// 该函数会self = shared_from_this(),
        /// 并将self保存到一个EventLoop任务中。
        bool asend(std::string && _data);

        /// async close。
        /// 如果返回true，则该函数会self = shared_from_this(),
        /// 并将self保存到一个EventLoop任务中。
        bool aclose(bool unlink_file = false);

        /// async connect
        /// 如果返回true，则该函数会self = shared_from_this(),
        /// 并将self保存到一个EventLoop任务中。
        /// 该函数会使用到POLLER的EVENTLOOP，因此需要先启动POLLER
        bool asyncConnect(std::string const &ip , int port , std::function<void(bool bret)>);

        /// async connect
        /// Unix域间套接字支持。
        bool asyncConnect(std::string & unixdoamin , std::function<void(bool bret)>);

        /// async connect
        /// 当然，其他结构（如IPV6），可以使用这个接口。
        /// 指定sockaddr_un时，请勿直接传递该结构的全部大小。
        bool asyncConnect(sockaddr * saddr , size_t saddr_size , std::function<void(bool bret)>);

        /// 返回内部Socket实例的引用.
        /// 实例生存期内，该指针永不为空。
        base::Socket * reference_handle();

        /// 返回内部repoller弱引用
        std::weak_ptr<RePoller> reference_repoller();

    private :

        /// 从一个Socket对象读数据(非阻塞读)。
        /// 返回值的ssize_t不小于0，因为内部已经处理了`EAGAIN || EWOULDBLOCK`的情况。
        base::KVPair<ssize_t,std::string> tryRead();

        ///监听ReadEvent
        void listenReadEvent(int fd);

    private :

        friend class SSLStream;

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