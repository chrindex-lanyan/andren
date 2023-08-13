
#pragma once

#include "../../base/andren_base.hh"


#include "../eventloop.hh"
#include "propoller.hh"


namespace chrindex::andren::network
{
    enum class SocketStreamEvents:int32_t
    {
        CLOSED = 0x00,     // 断开连接 recv() == 0
        READABLE = 0x01,   // 读完成   recv() > 0 
        WRITABLE = 0x10,   // 连接成功或者写事件到达
    };

    class TcpStream 
        : public std::enable_shared_from_this<TcpStream> , public base::noncopyable
    {
    public :

        using OnConnected = std::function<void(int status)>;
        using OnClose = std::function<void()>;
        using OnReadDone  = std::function<void(ssize_t ret, std::string && data)>;
        using OnWriteDone = std::function<void(ssize_t ret, std::string && lastData)>;

        TcpStream();
        TcpStream(base::Socket && sock);
        TcpStream(TcpStream &&_) ;
        ~TcpStream();

        void operator=(TcpStream && _);

        bool setOnClose(OnClose onClose);

        bool reqConnect(std::string ip, int32_t port, OnConnected onConnected );

        std::shared_ptr<TcpStream> accept();

        /// @brief 
        /// @param onRead 当ssize_t ret是-2时是操作步骤出错，-3是请求超时。
        /// @param timeoutMsec 请求时限。超过该时限请求视为超时，除非有数据。
        /// @return 
        bool reqRead(OnReadDone onRead, int timeoutMsec = 15000);

        /// @brief 
        /// @param data 
        /// @param onWrite 当ssize_t ret是-2时是操作步骤出错，-3是请求超时。
        /// @return 
        bool reqWrite(std::string const & data , OnWriteDone onWrite);

        bool reqWrite(std::string && data , OnWriteDone onWrite);

        base::Socket * handle();

        bool valid()const ;

        /// ###### OpenSSL 

        /// @brief 开启SSL。
        /// 需要在Socket FD有效。
        /// @param assl 
        /// @return 
        bool enableSSL(base::aSSL && assl);

        base::aSSLSocketIO * sslioHandle();

        bool usingSSL() const;

        /// ###### EventLoop && Epool

        /// @brief 设置Poller
        /// @param pp 
        bool setProPoller(std::weak_ptr<ProPoller> pp  );

        void setEventLoop(std::weak_ptr<EventLoop> ev);

        /// @brief 断开连接并返回套接字
        /// 请注意，断开连接不等于套接字被关闭。
        /// 调用此函数会导致套接字Read时返回0，预示连接
        /// 已经断开，然后用户会收到OnClose回调。
        /// 请在该回调进行清理工作。
        void disconnect();

    private :

        base::KVPair<ssize_t,std::string> tryRead(base::Socket & sock);

    private :

        struct _Private
        {
            _Private();
            ~_Private();

            base::Socket m_socket;
            base::aSSLSocketIO m_asslio;

            OnClose m_onClose;

            // #### 

            std::weak_ptr<EventLoop> m_ev;
            std::weak_ptr<ProPoller> m_pp;
        } ;

        std::unique_ptr<_Private> pdata;

    };


}


