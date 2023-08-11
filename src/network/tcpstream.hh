
#pragma once

#include "../base/andren_base.hh"


#include "eventloop.hh"
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

        using OnConnected = std::function<void(std::shared_ptr<TcpStream> )>;
        using OnClose = std::function<void()>;
        using OnReadDone  = std::function<void(ssize_t ret, std::string const & data)>;
        using OnWriteDone = std::function<void(ssize_t ret, std::string const & lastData)>;

        TcpStream();
        TcpStream(base::Socket && sock);
        TcpStream(TcpStream &&_) ;
        ~TcpStream();

        void operator=(TcpStream && _);

        bool setOnClose(OnClose onClose);

        bool reqConnect(std::string ip, int32_t port, OnConnected onConnected );

        std::shared_ptr<TcpStream> accept();

        bool reqRead(OnReadDone onRead, int timeoutMsec = 15000);

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

        /// @brief 该函数会订阅可读事件
        /// @param pp 
        void setProPoller(std::weak_ptr<ProPoller> pp);

        void setEventLoop(std::weak_ptr<EventLoop> ev);


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


