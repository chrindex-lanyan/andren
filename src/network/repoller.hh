#pragma once

#include "../base/andren_base.hh"

#include "eventloop.hh"
#include <atomic>
#include <functional>
#include <map>
#include <memory>


namespace chrindex::andren::network
{

    class RePoller : public std::enable_shared_from_this<RePoller> , base::noncopyable
    {
    public :
        RePoller();
        RePoller(RePoller &&) = delete;
        ~RePoller();

        using OnEventUP = std::function<void(int events)>;

        /// 请求开始polling。
        /// 函数返回true时，请求被接受，且会使用shared_from_this()保证本实例的生命周期。
        /// 如果EventLoop无效或者不可用，则立即返回false。
        /// epollWaitPerTick_msec指每次epoll wait的超时时间。
        /// 如果设置的过大将影响IO线程的其他任务的执行。
        /// 建议设置为1~10 MSEC.
        bool start(std::weak_ptr<EventLoop> wev, int epollWaitPerTick_msec);

        /// 设置polling标志为shutdown，此操作会让下一次polling结束后，不再继续polling。
        void stop();

        std::weak_ptr<EventLoop> eventLoopReference() const;

     public:
        /// ##### Note 1 下列函数仅能在IO线程被调用

        /// 添加一个FD和要监听的事件
        /// 返回false，很可能是FD已存在。
        bool append( int fd , int events );

        /// 修改某个FD要监听的事件类型
        /// 返回false，很可能是FD不存在。
        bool modify( int fd , int events );

        /// 删除对某个FD的事件监听。
        /// 注意，该函数也会清除FD对应的回调函数。
        bool cancle( int fd );

        /// 为一个fd手动发送一个事件，而不是依赖于EPOLL_WAIT。
        /// 该FD可以是系统FD（FD > 0），也可以是自定义FD（FD <= -2）。
        /// 如果传递的FD值为0或者-1，函数立即返回false；
        /// 如果返回了false，说明没有FD能够接收此事件；
        /// 如果返回true，则事件被投递成功。
        /// 事件的定义和Epoll的event一致。
        /// 如果epoll_wait出该fd的events，则该events会和epoll的events合并，
        /// 否则只会触发此events。
        /// 在调用此函数前，需要注意所关心的FD是否已经被setReadyCallback()。
        bool notifyEvents(int fd , int events); // 自定义信号

        /// 设置感兴趣的FD，其事件触发时的回调。
        /// 该FD可以是Linux中有效的FD（FD > 0），也可以是用户自定义的FD（FD <= -2），
        /// 只有FD>0时，该FD会被EPOLL_CTL_ADD；FD不可以是0或者-1。
        /// 注意，该回调是持久化的，注意不要保存REPOLLER的强引用实例，
        /// 除非你调用了cancle。
        bool setReadyCallback( int fd , OnEventUP && onEventUP );

        /// 设置感兴趣的FD，其事件触发时的回调。
        /// 函数调用bool setReadyCallback( int fd , OnEventUP && onEventUP)函数
        bool setReadyCallback( int fd , OnEventUP const & onEventUP );

        /// End Note 1

    public :
        /// ##### Note 2 下列函数可以跨线程使用，但回调函数被调用时，一定是在IO线程被回调。
        /// 这两个函数还将调用shared_from_this()保证自身的声明周期。

        /// 保存一个任意对象到内部。
        /// 当返回true时，请求被接受，并在以后被执行。
        /// 无论执行请求是否成功，onSave都会被调用。
        /// @param id 对象ID。
        /// @param force 是否强制覆盖FD，当且仅当FD出现重复时。
        /// 否则不会覆盖。
        /// @param _object 要保存的对象。函数不会判断其是否has_value()。
        /// @param onSave 保存完成回调。onSave函数不能为空，否则接口立即返回false。
        /// 当ret为true时保存成功。
        /// 如果force为false但FD出现重复，则ret为false。
        /// 无论情况如何，*obj绝不为nullptr，且value与_object的value一致。
        /// @return 请求是否已被接受。true为已接受。
        bool saveObject(int id , bool force , std::any _object,  std::function<void(bool ret, std::any * _obj)> onSave);

        /// 查找一个任意对象从内部。
        /// 当返回true时，请求被接受，并在以后被执行。
        /// 无论执行请求是否成功，onFind都会被调用。
        /// @param id 对象ID。
        /// @param takeOrNot 是否取出。
        /// true为取出，即本实例将在回调onFind完成后放弃所要查找的对象的所有权，
        /// 请注意转移对象所有权。false为仅引用。
        /// @param onFind 查找完成回调。onFind函数可以为空，在此情况下onFind将不会被调用。
        /// 当ret为true时查找成功，此时*obj不为nullptr。
        /// @return 请求是否已被接受。true为已接受。
        bool findObject(int id , bool takeOrNot ,  std::function<void(bool ret, std::any * _obj)> onFind );

        /// ##### End Note 2

    private:

        void workNextTick(int epollWaitPerTick_msec);

        void work(int epollWaitPerTick_msec);

    private :

        struct _private 
        {
            std::atomic<bool> m_shutdown;
            std::map<int, std::any>  m_objects; // 对象池。
            std::map<int, OnEventUP> m_callbacks;
            std::map<int,int>        m_customEvents; // 需要手动发送的events
            base::Epoll m_ep;
            std::weak_ptr<EventLoop> m_wev;
        };

        std::unique_ptr<_private> data;

    };




}