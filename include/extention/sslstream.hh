#pragma once

#include <functional>
#include <memory>

#include "socket.hh"
#include "ssl.hh"

#include "sockstream.hh"
#include "repoller.hh"

namespace chrindex::andren::network
{

    /// 该类在SockStream的基础上支持了OpenSSL IO 。
    /// 也可以在不使用OpenSSL的情况下使用，这时功能和SockStream无异。
    class SSLStream: public std::enable_shared_from_this<SSLStream> , base::noncopyable
    {
    public :

        /// 当可读且读取完成时
        using OnRead  = std::function<void(ssize_t ret , std::string &&)>;

        /// 当发送完成时
        using OnWrite = std::function<void(ssize_t ret , std::string &&)>;

        /// 当关闭时
        using OnClose = std::function<void()>;

        SSLStream();
        SSLStream(base::Socket && sock , std::weak_ptr<RePoller> wrp);
        SSLStream(SockStream && sock);
        SSLStream(SSLStream&&) noexcept;
        ~SSLStream();

        void operator=(SSLStream && _) noexcept;

        /// 使用SSL。
        /// 要求调用者提供已经配置好的SSL对象。
        /// 同时提供端类型。endType==1时认为作为服务端连接。
        /// 当endType==2时作为客户端连接。
        bool usingSSL( base::aSSL && assl , int endType);

        /// 查询是否启用了OpenSSL
        bool enabledSSL() const ;

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
        bool aclose();

        /// async connect
        /// 如果返回true，则该函数会self = shared_from_this(),
        /// 并将self保存到一个EventLoop任务中。
        /// 该函数会使用到POLLER的EVENTLOOP，因此需要先启动POLLER
        bool asyncConnect(std::string const &ip , int port , std::function<void(bool bret)>);

        /// 返回内部Socket实例的引用.
        /// 实例生存期内，该指针永不为空。
        base::Socket * reference_handle();

        /// 返回内部repoller弱引用
        std::weak_ptr<RePoller> reference_repoller();

        /// 返回内部SSLIO的实例指针。
        /// 该指针可能为空，当未曾调用usingSSL时时。
        base::aSSLSocketIO * reference_sslio();

        

    private :

        /// 从一个Socket对象读数据(非阻塞读)。
        /// 返回值的ssize_t不小于0，因为内部已经处理了`EAGAIN || EWOULDBLOCK`的情况。
        base::KVPair<ssize_t,std::string> tryRead();

        /// 监听ReadEvent
        void listenReadEvent(int fd);

        /// 实际读取数据
        base::KVPair<ssize_t , std::string> real_read();

        /// 实际写入数据
        ssize_t real_write(char const * data , size_t size , int flags);

        /// 实际关闭连接
        void real_close();

    private :

        struct _private 
        { 
            OnClose m_onClose;
            OnRead m_onRead;
            OnWrite m_onWrite;
            base::Socket m_sock;
            base::aSSLSocketIO m_sslio;
            std::weak_ptr<RePoller> wrp;
            bool m_usedSSL;
            bool m_sslshutdown;
        };
        std::unique_ptr<_private> data;
    };

}