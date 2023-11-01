#pragma once

#include <string>

#include <semaphore.h>

#include <functional>

#include "noncopyable.hpp"

/**
 * @brief 提供两种信号量
 * 1. POSIX 内存信号量，用于线程之间同步。
 * 2. POSIX 具名信号量，用于线程或者进程之间同步。
 *
 */

namespace chrindex::andren::base
{
    class PosixSigHandle
    {
    public:
        PosixSigHandle() {}
        virtual ~PosixSigHandle() {}

        virtual sem_t *sem() = 0;

        // 判断类型匹配
        virtual int typeno() const = 0;
    };
    using psig_handle = PosixSigHandle;

    class PosixNamedSignal : public PosixSigHandle, public noncopyable
    {
    public:
        // 只做结构清空操作
        PosixNamedSignal();

        // 注意，本析构函数只调用unlink
        ~PosixNamedSignal();

        PosixNamedSignal(PosixNamedSignal &&);

        PosixNamedSignal &operator=(PosixNamedSignal &&);

        // 打开或创建。
        bool open(const std::string &name, int flags,
                  std::function<void(int errNO)> whenSEM_FAILED = {}, unsigned int mode = (~0), unsigned int value = (~0));

        // 关闭信号量。
        void close();

        int clockWait(unsigned int msec);

        // 解除于信号量的联系，但不删除它。
        int unlink();

        sem_t *sem() override;

        std::string name() const;

        // 判断类型匹配
        int typeno() const override;

        sem_t *m_sem;
        std::string m_name;
    };
    using pnsig = PosixNamedSignal;

    class PosixUnnamedSignal : public PosixSigHandle, public noncopyable
    {
    public:
        // 该构造会调用reinit()
        PosixUnnamedSignal(int value);

        // 该析构会调用destroy()
        ~PosixUnnamedSignal();

        PosixUnnamedSignal(PosixUnnamedSignal &&);

        PosixUnnamedSignal &operator=(PosixUnnamedSignal &&);

        sem_t *sem() override;

        // 判断类型匹配
        int typeno() const override;

        sem_t *m_sem;
    };
    using pusig = PosixUnnamedSignal;

    /// 注意管理PosixSigHandle所有权。
    /// 本类只借用。 
    class PosixSignal : public noncopyable
    {
    public:
        // 仅指针置空
        PosixSignal();

        // 不做任何事
        ~PosixSignal();

        PosixSignal(PosixSignal &&);

        PosixSignal &operator=(PosixSignal &&);

        void borrow_by(PosixSigHandle *handle);

        int wait();

        int tryWait();

        int timeWait(int msec);

        int post();

        int value(int &val);

        bool valid() const;

        PosixSigHandle *handle() const;

    private:
        PosixSigHandle *m_handle;
        int m_type;
    };
    using psig = PosixSignal;

}
