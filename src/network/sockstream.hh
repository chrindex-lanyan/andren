#pragma once

#include "../base/andren_base.hh"

#include "repoller.hh"
#include <functional>
#include <memory>


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

        SockStream();
        SockStream(base::Socket && sock, std::weak_ptr<RePoller> wrp);
        ~SockStream();

        void setOnClose(OnClose const & cb);

        void setOnRead(OnRead const & cb);

        void setOnWrite(OnWrite const & cb);

        void setOnClose(OnClose && cb);

        void setOnRead(OnRead && cb);

        void setOnWrite(OnWrite && cb);

        bool startListenReadEvent();

        /// async send
        bool asend(std::string const & _data);

        /// async send
        bool asend(std::string && _data);

        /// async close
        void aclose();

    private :
        static base::KVPair<ssize_t,std::string> tryRead(base::Socket & sock);

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