#pragma once

#include "tcpstream.hh"

#include "eventloop.hh"

namespace chrindex::andren::network
{

    enum class TSM_Error : int32_t
    {
        OK = 0 , 
        STOPPED = -1,       // 停止了
        ACCEPT_FAILED = -2, // ACCEPT 失败
        SET_SOCKET_OPT_FAILED = -3,
        BIND_ADDR_FAILED=-4,
        LISTEN_SOCKET_FAILED = -5,
        SERVER_SOCKET_INVALID = -6,
        ACCEPT_CALLBACK_EMPTY = -7,
        EVENTLOOP_ADD_TASK_FAILED = -8,
    };

    class TcpStreamManager : public std::enable_shared_from_this<TcpStreamManager>, base::noncopyable
    {
    public:
        using OnAccept = std::function<void(std::shared_ptr<TcpStream> stream, TSM_Error status)>;

        TcpStreamManager();

        ~TcpStreamManager();

        void setEventLoop(std::weak_ptr<EventLoop> ev);

        TSM_Error start(std::string const &ip, uint32_t port);

        TSM_Error requestAccept(OnAccept onAccept);

        void stop();

    private:
        std::weak_ptr<EventLoop> m_ev;
        std::shared_ptr<base::Epoll> m_ep;
        TcpStream m_server;
        std::atomic<bool> m_stop;
    };

}
