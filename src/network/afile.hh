#pragma once


#include "../base/andren_base.hh"
#include <functional>
#include <memory>

#include "task_distributor.hh"



namespace chrindex::andren::network
{
    /// 异步文件IO。
    /// 实际上仍是阻塞IO，但是用EventLoop模拟了异步IO。
    /// 该异步IO是Proactor模式的。
    /// 为了保证写入的顺序性，且不在类中引入额外的成员，
    /// 该类的所有操作都严格遵守：所有操作在IO线程里完成。
    /// 请注意，该类的生命周期是在EventLoop中仅持续到请求完成，
    /// 因此用户需要自己保存AFile的生命周期。
    /// 我们要求AFile必须是std::shared_ptr<AFile>实例的。

    class AFile : public std::enable_shared_from_this<AFile> , base::noncopyable
    {
    public :

        using OnControl  = std::function<void(bool isOpen)>;
        using OnWriteDone = std::function<void(ssize_t,std::string &&)> ;
        using OnReadDone = std::function<void(ssize_t , std::string &&)>;

        AFile();

        ~AFile();

        void setEventLoop(std::weak_ptr<TaskDistributor> wev);

        /// open文件。
        /// 提供了这个接口，用于AFile实例刚刚创建的时期。
        /// 该时期所有的读写操作还没开始，因此可以不使用AsyncOpen。
        /// flags指读写权限，createMode是文件创建时的选项。
        /// 该函数在Open完成时设置FD为非阻塞IO。
        bool open(std::string const & path, int flags, int createMode = -1);

        /// close文件。
        /// see `bool aclose(OnControl && onControl);`
        bool aclose(OnControl const& onControl);

        /// close文件。
        /// 该操作是异步的，在IO线程里完成。
        bool aclose(OnControl && onControl);

        /// open文件.
        /// 该操作是异步的，在IO线程里完成。
        bool areopen(OnControl const & onControl, std::string const & path, int flags, int createMode =-1);

        /// open文件.
        /// 该操作是异步的，在IO线程里完成。
        bool areopen(OnControl && onControl, std::string && path, int flags, int createMode =-1);

        /// 发送一个写请求。
        /// 写操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        bool asend(std::string const & data , OnWriteDone const & onWriteDone);

        /// 发送一个写请求。
        /// 写操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        bool asend(std::string && data , OnWriteDone && onWriteDone);

        /// 发送一个读请求。
        /// 读操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        /// 请注意，read必须是非阻塞的。
        bool aread(OnReadDone const & onReadDone);

        /// 发送一个读请求。
        /// 读操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        /// 请注意，read必须是非阻塞的。
        bool aread(OnReadDone && onReadDone);

        base::File & handle();

    private :

        bool switchNonblock(bool blocking);


    private :
        base::File m_file;
        std::weak_ptr<TaskDistributor> m_wev;
    };


};

