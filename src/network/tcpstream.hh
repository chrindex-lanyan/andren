
#pragma once

#include "../base/andren_base.hh"


#include "eventloop.hh"



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

        using OnClose = std::function<void()>;
        using OnReadDone  = std::function<void(ssize_t ret, std::string const & data)>;
        using OnWriteDone = std::function<void(ssize_t ret, std::string const & lastData)>;

        TcpStream();
        TcpStream(base::Socket && sock);
        TcpStream(TcpStream &&_) ;
        ~TcpStream();

        void operator=(TcpStream && _);

        bool setOnClose(OnClose onClose);

        bool connect(std::string ip, int32_t port, bool aSync);

        std::shared_ptr<TcpStream> accept();

        bool reqRead(OnReadDone onRead);

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

        void setEpoll(std::weak_ptr<base::Epoll> ep);

        void setEventLoop(std::weak_ptr<EventLoop> ev);

        int  notify(bool bread, bool bwrite, bool bexcept);

    private :

        base::KVPair<ssize_t,std::string> tryRead(base::Socket & sock);

        bool processEvents(int events);

    private :

        struct _Private
        {
            _Private();
            ~_Private();

            base::Socket m_socket;
            base::aSSLSocketIO m_asslio;

            std::string  m_wrbuffer;
            OnClose m_onClose;
            OnReadDone m_onRead;
            OnWriteDone m_onWrite;

            // #### 

            std::weak_ptr<EventLoop> m_ev;
            std::weak_ptr<base::Epoll> m_ep;
        } ;

        std::unique_ptr<_Private> pdata;

    };


}


