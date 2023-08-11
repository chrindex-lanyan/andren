#pragma once

#include "../base/andren_base.hh"

#include "eventloop.hh"

namespace chrindex::andren::network
{
    struct ProEvent
    {
        int readable : 1;
        int writeable : 1;
        int except : 1;
        int reserv : 29;

        ProEvent();

        void cover(int32_t epoll_event_events);

        int32_t cover();
    };

    class ProPoller : public std::enable_shared_from_this<ProPoller>, base::noncopyable
    {
    public:
        ProPoller();

        ~ProPoller();

        void setEventLoop(std::weak_ptr<EventLoop> ev);

        void setEpoller(std::weak_ptr<base::Epoll> ep);

        /// @brief 获取Epoll的弱引用。
        /// 请注意，ProPoller也不持有Epoll的所有权。
        /// @return 
        std::weak_ptr<base::Epoll> epollHandle();

        bool valid() const;

        /// @brief 开始polling。
        /// 当且仅当EventLoop在运行时。
        /// @return true为成功，无需等待EventLoop启动。
        bool start();

    public:

        /// @brief 设置事件可缓存
        /// 该函数要运行在IO线程中
        /// @param fd 文件描述符
        /// @param proev 事件类型
        /// @return true/false
        bool subscribe(int fd, ProEvent proev);

        /// @brief 修改事件缓存类型
        /// 该函数要运行在IO线程中
        /// @param fd 文件描述符
        /// @param proev 事件类型
        /// @return true/false
        bool modSubscribe(int fd, ProEvent proev);

        /// @brief 取消事件可缓存
        /// 该函数要运行在IO线程中
        /// @param fd 文件描述符
        /// @return true/false
        bool delSubscribe(int fd);

        /// @brief 清除所有已被缓存的事件
        /// 该函数要运行在IO线程中
        void clearCache();

        /// @brief 等待事件
        /// 当事件被缓存时，cb被调用。
        /// 该函数要运行在IO线程中.
        /// 该函数保证，只要查到事件，则无论是否超时都将isTimeout设置为fasle。
        /// @param fd 文件描述符
        /// @param cb 回调
        /// @param timeoutMsec 超时
        void findAndWait(int32_t fd , std::function<void (ProEvent , bool isTimeout)> cb, int64_t timeoutMsec);

    private:

        void processEvents();

        bool updateEpoll(int fd, ProEvent proev, base::EpollCTRL type);

    private:
        std::weak_ptr<EventLoop> m_ev;
        std::weak_ptr<base::Epoll> m_ep;
        std::map<int32_t, ProEvent> m_cacheEvents;
        base::EventContain m_evc;
    };

}
