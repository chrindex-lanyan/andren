#pragma once

#include <string>

#include <semaphore.h>

#include <functional>

/**
 * @brief 提供两种信号量
 * 1. POSIX 内存信号量，用于线程之间同步。
 * 2. POSIX 具名信号量，用于线程或者进程之间同步。
 * 3. ASIG  自带的信号管理机制，用于线程之间同步。
 * 
 * 
 * 以下列出的信号列表仅适用于POSIX信号。对于ASIG，下列的信号仅作信号值供用户处理，不会有任何默认操作。
 *  信号      取值          默认动作   含义（发出信号的原因）
    SIGHUP      1           Term    终端的挂断或进程死亡  
    SIGINT      2           Term    来自键盘的中断信号 
    SIGQUIT     3           Core    来自键盘的离开信号 
    SIGILL      4           Core    非法指令 
    SIGABRT     6           Core    来自abort的异常信号 
    SIGFPE      8           Core    浮点例外 
    SIGKILL     9           Term    杀死 
    SIGSEGV     11          Core    段非法错误(内存引用无效) 
    SIGPIPE     13          Term    管道损坏：向一个没有读进程的管道写数据 
    SIGALRM     14          Term    来自alarm的计时器到时信号 
    SIGTERM     15          Term    终止 
    SIGUSR1     30,10,16    Term    用户自定义信号1 
    SIGUSR2     31,12,17    Term    用户自定义信号2 
    SIGCHLD     20,17,18    Ign     子进程停止或终止 
    SIGCONT     19,18,25    Cont    如果停止，继续执行
    SIGSTOP     17,19,23    Stop    非来自终端的停止信号
    SIGTSTP     18,20,24    Stop    来自终端的停止信号
    SIGTTIN     21,21,26    Stop    后台进程读终端
    SIGTTOU     22,22,27    Stop    后台进程写终端

    SIGBUS      10,7,10     Core    总线错误（内存访问错误）
    SIGPOLL                 Term    Pollable事件发生(Sys V)，与SIGIO同义
    SIGPROF     27,27,29    Term    统计分布图用计时器到时
    SIGSYS      12,-,12     Core    非法系统调用(SVr4)
    SIGTRAP     5           Core    跟踪/断点自陷
    SIGURG      16,23,21    Ign     socket紧急信号(4.2BSD)
    SIGVTALRM   26,26,28    Term    虚拟计时器到时(4.2BSD)
    SIGXCPU     24,24,30    Core    超过CPU时限(4.2BSD)
    SIGXFSZ     25,25,31    Core    超过文件长度限制(4.2BSD)

    SIGIOT      6           Core    IOT自陷，与SIGABRT同义
    SIGEMT      7,-,7       Term
    SIGSTKFLT   -,16,-      Term    协处理器堆栈错误(不使用)
    SIGIO       23,29,22    Term    描述符上可以进行I/O操作
    SIGCLD      -,-,18      Ign     与SIGCHLD同义
    SIGPWR      29,30,19    Term    电力故障(System V)
    SIGINFO     29,-,-              与SIGPWR同义
    SIGLOST     -,-,-       Term    文件锁丢失
    SIGWINCH    28,28,20    Ign     窗口大小改变(4.3BSD, Sun)
    SIGUNUSED   -,31,-      Term    未使用信号(will be SIGSYS)
 */

namespace chrindex::andren::base
{

    class PosixSigHandle
    {
    public :
        PosixSigHandle(){}
        virtual ~PosixSigHandle (){}

        virtual sem_t * sem() = 0;

        // 判断类型匹配
        virtual int typeno()const =0;
    };


    class PosixNamedSignal : public PosixSigHandle
    {
        public:
        // 只做结构清空操作
        PosixNamedSignal();

        // 注意，本析构函数只调用unlink
        ~PosixNamedSignal();

        // 打开或创建。
        bool open(const std::string &name ,
            int flags , unsigned int mode = (~0) , unsigned int value = (~0));

        // 关闭信号量。
        void close ();

        int clockWait(unsigned int msec);

        // 解除于信号量的联系，但不删除它。
        int unlink();

        sem_t * sem() override;

        std::string name()const;

        // 判断类型匹配
        int typeno() const override ;

        sem_t *m_sem;
        std::string m_name;
    };

    class PosixUnnamedSignal : public PosixSigHandle
    {
        public :
        // 该构造会调用reinit()
        PosixUnnamedSignal(int value = 0); 

        // 该析构会调用destroy()
        ~PosixUnnamedSignal();

        sem_t * sem() override;

        // 判断类型匹配
        int typeno()const override;

        sem_t m_sem;
    };

    class PosixSignal
    {
    public :

    // 仅指针置空
    PosixSignal();

    // 不做任何事
    ~PosixSignal();

    void setHandle(PosixSigHandle * handle);

    int wait();

    int tryWait();

    int timeWait(int msec);

    int post();

    int value(int & val);

    bool valid() const;

    private :
        PosixSigHandle * m_handle;
        int m_type;
    };


    class ASigPost
    {
    public :

        ASigPost();
        ~ASigPost();

        void setSigNo(int sig);

        void post(int size = 1);

    private :
        int m_sig;
    };

    class ASigManager;

    class ASigWait
    {
    public :

        using callback_t = std::function <void (int sig, bool waitDoneOrNot)>;

        ASigWait();
        ~ASigWait();

        ASigWait& setSigNo(int sig);

        ASigWait* setCallback(callback_t);

        // 将会立即提交wait请求，交由ASIG-MANAGER处理
        // 处理完成后调用回调
        void tryAsyncWait(int msec = 0, ASigManager * _pManager);

        // 取消信号订阅。本对象不再接受该信号。
        void destroySigWait(ASigManager * _pManager);

    private:
        int m_sig;
        callback_t m_func;
    };

    class ASigManager
    {
        public :
        ASigManager();
        ~ASigManager();
    };

}
