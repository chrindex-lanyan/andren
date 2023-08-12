#pragma once

#include "../base/andren_base.hh"

#include "eventloop.hh"

namespace chrindex::andren::network
{
    enum class ProPollerError:int
    {
        OK = 1,                             // 操作成功
        FD_NOTFOUND_IN_CACHE,               // FD的未曾被操作
        FD_FOUND_BUT_EVENT_NOFOUND,         // FD被发现操作过，但未对指定的事件监听
        FD_AND_EVENT_FOUND_BUT_NO_OCCURRED, // FD的事件已添加监听，但事件已超时且未发生

    };

    class ProPoller : public std::enable_shared_from_this<ProPoller>, base::noncopyable
    {
    public:
        ProPoller();

        ~ProPoller();

        /// @brief Poller是否可用（即使EventLoop未运行）
        /// @return 
        bool valid() const;

        /// @brief 开始polling。
        /// 当且仅当EventLoop可用（即使未启动）。
        /// @return true为成功。
        bool start(std::shared_ptr<EventLoop> ev);

    public:

    /// ########  AAA :  以下函数只能在EventLoop的IO线程内被调用。 #####
        
        /// @brief 为FD添加事件监听。
        /// @param fd 
        /// @param events 感兴趣的事件。
        /// 可读事件 ： EPOLLIN , 可写 : EPOLLOUT
        /// @return 
        bool addEvent(int fd , int events );

        /// @brief 修改FD监听的事件。
        /// 注意，如果没有add，则会add。
        /// @param fd 
        /// @param events 感兴趣的事件
        /// @return 
        bool modEvent(int fd , int events );

        /// @brief 删除某个FD的事件监听
        /// @param fd 
        /// @return 
        bool delEvent(int fd  );

        /// @brief 查询最后一次的操作历史。
        /// @param fd 
        /// @return 返回事件集。如果-1则未监听FD。
        int findEvent(int fd);

        /// @brief 查询最后一次Wait的结果。
        /// @param fd 
        /// @return 返回事件集。如果-1则未查询到。
        int findLastEvent(int fd);

        using OnFind = std::function<void(ProPollerError errcode)>;

        /// @brief 等待FD中某个感兴趣的事件，直到超时，或者等待到。
        /// @param fd 
        /// @param events 
        /// @param timeoutMsec 毫秒。不建议太低。
        /// @param cb 回调函数。要求必须有效。
        /// @return 操作是否成功。true为成功。
        bool findAndWait( int fd , int events , int timeoutMsec, EventLoop* wev , OnFind cb);

    /// ########  END AAA #### 

    private:

        bool processEvents(std::weak_ptr<EventLoop> ev);

        /// @brief 更改Epoll
        /// @param fd 
        /// @param events 
        /// @return 
        bool realUpdateEvents(int fd, int events, base::EpollCTRL ctrl);

    private:
        std::atomic<bool> m_exit;
        base::Epoll m_epoll;
        std::map<int , int> m_cache; // epoll control 操作的记录
        std::map<int , int> m_lastWait; // 上次epoll wait出来的记录
    };

}
