

#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "noncopyable.hpp"

/// 前向声明

struct sockaddr;
struct msghdr;
struct mmsghdr;
struct timespec;
struct timeval;


namespace chrindex::andren::base
{

    class Socket : public noncopyable
    {
    public:
        /// @brief 默认构造。提供一个无效socket
        Socket();

        /// @brief 直接构造。成功后socket对象持有有效socket fd。
        /// @param domain 套接域
        /// @param type 流类型
        /// @param protocol 协议类型
        Socket(int domain, int type, int protocol);

        /// @brief 析构。对有效的socket fd进行close()
        ~Socket();

        /// @brief 移动构造
        /// @param _ 被移走的对象
        Socket(Socket &&_);

        /// @brief 移动赋值
        /// @param _ 被移走的对象
        /// @return 自身的引用
        Socket &operator=(Socket &&_);

        /// @brief 判断socket fd是否被成功初始化，无论现在是否可用。
        /// @return 仅fd有效时返回true。
        bool valid() const;

        /// @brief 托管一个已存在的FD。要求本实例必须满足call (bool valid() = false)的。
        /// @param fd 已存在的fd
        /// @return 如果本实例已经存在有效fd，则托管失败且立即返回-1，否则返回0。
        int managedFD(int fd);

        /// @brief 为socket fd绑定socket 地址。
        /// @param addr 地址指针
        /// @param addrlen 地址结构大小
        /// @return 成功时，返回0。发生错误时，返回-1，并设置errno以指示错误。
        int bind(struct sockaddr const *addr,
                 uint32_t addrlen);

        /// @brief 本fd作为监听fd时，尝试接受一个传入的连接。
        /// @param addr 用于保存传入连接地址的地址结构指针
        /// @param addrlen 地址结构的大小
        /// @return 成功时，返回接受的套接字的文件描述符（一个非负整数）。
        /// 发生错误时，返回-1，并设置errno以指示错误，addrlen保持不变。
        int accept(struct sockaddr *addr,
                   uint32_t *addrlen);

        /// @brief 本fd作为监听fd时，尝试接受一个传入的连接。
        /// 与accept()相比，该函数增加了一个flags参数。
        /// @param addr 同int accept(...)
        /// @param addrlen 同int accept(...)
        /// @param flags 标志
        /// @return 成功时，返回接受的套接字的文件描述符（一个非负整数）。
        /// 发生错误时，返回-1，并设置errno以指示错误，addrlen保持不变。
        int accept4(struct sockaddr *addr,
                    uint32_t *addrlen, int flags);

        /// @brief 使本socket fd连接到远端
        /// @param addr 远端地址
        /// @param addrlen 地址结构长度
        /// @return 如果连接或绑定成功，则返回0。发生错误时，返回-1，并设置errno以指示错误。
        int connect(struct sockaddr const *addr,
                    uint32_t addrlen);

        /// @brief 文件句柄控制函数。控制socket fd。
        /// @param cmd 控制命令
        /// @return 对于成功的调用，返回值取决于操作的类型：
        /// F_DUPFD：新的文件描述符。
        /// F_GETFD：文件描述符的标志值。
        /// F_GETFL：文件状态标志的值。
        /// F_GETLEASE：文件描述符上持有的租约类型。
        /// F_GETOWN：文件描述符的所有者。
        /// F_GETSIG：当读取或写入变为可能时发送的信号值，或者对于传统的SIGIO行为，返回值为零。
        /// F_GETPIPE_SZ、F_SETPIPE_SZ：管道容量。
        /// F_GET_SEALS：一个位掩码，用于标识已为文件描述符引用的inode设置的封印（seal）。
        /// 其他所有命令：返回值为零。
        /// 发生错误时，返回值为-1，并设置errno以指示错误。
        int fcntl(int cmd);

        /// @brief 文件句柄控制函数。控制socket fd。
        /// @param cmd 控制命令
        /// @param arg 命令参数
        /// @return 见 int fcntl(...)
        int fcntl(int cmd, long arg);

        /// @brief 用于获取连接套接字的远程（对等方）地址信息。
        /// @param addr 指向存储对等方地址信息的结构体的指针。
        /// @param addrlen 指向存储对等方地址信息结构体大小的指针。
        /// @return 成功时返回 0，失败时返回 -1。
        int getpeername(struct sockaddr *addr,
                        uint32_t *addrlen);

        /// @brief 用于获取套接字的本地地址信息。
        /// @param addr 指向存储本地地址信息的结构体的指针。
        /// @param addrlen 指向存储本地地址信息结构体大小的指针。
        /// @return 成功时返回 0，失败时返回 -1。
        int getsockname(struct sockaddr *addr,
                        uint32_t *addrlen);

        /// @brief 用于获取套接字选项的值。
        /// @param level 选项所在的协议层或套接字层。
        /// @param optname 选项的名称。
        /// @param optval 指向存储选项值的缓冲区的指针。
        /// @param optlen 指向存储选项值缓冲区大小的指针。
        /// @return 成功时返回 0，失败时返回 -1。
        int getsockopt(int level, int optname,
                       void *optval,
                       uint32_t *optlen);

        /// @brief 用于设置套接字选项的值。
        /// @param level 选项所在的协议层或套接字层。
        /// @param optname 选项的名称。
        /// @param optval 指向包含选项值的缓冲区的指针。
        /// @param optlen 选项值的长度。
        /// @return
        int setsockopt(int level, int optname,
                       void const *optval,
                       uint32_t optlen);

        /// @brief 用于对套接字执行特定的操作，通常用于设置或获取与底层设备相关的参数。
        /// @param request 操作的请求码。
        /// @param arg 请求的参数。
        /// @return 操作的返回值，成功时返回非负值，失败时返回 -1。
        int ioctl(unsigned long request, ...);

        /// @brief  函数将 sockfd 所指向的套接字标记为被动套接字，即用于接受传入的连接请求，使用 accept(2) 函数来处理。
        /// sockfd 参数是一个文件描述符，引用了一个类型为 SOCK_STREAM 或 SOCK_SEQPACKET 的套接字。
        /// backlog 参数定义了等待连接队列（pending connections queue）的最大长度。如果连接请求在队列已满时到达，客户端可能会收到一个带有 ECONNREFUSED 错误指示的错误，
        /// 或者如果底层协议支持重传，则可能会忽略请求，以便稍后重新尝试连接成功。
        /// 请注意，listen() 函数仅适用于支持连接的套接字类型，如 TCP 套接字。
        /// 对于其他类型的套接字，如 UDP 套接字，不需要调用 listen() 函数。
        /// @param backlog 等待连接队列的最大长度。
        /// @return 成功时，返回值为零。发生错误时，返回值为-1，并设置errno以指示错误。
        int listen(int backlog);

        /// @brief 从指定的套接字接收数据，多用于TCP。
        /// @param buf 指向接收数据的缓冲区的指针。
        /// @param len 缓冲区的长度。
        /// @param flags 可选的标志参数，用于指定接收操作的选项，如 MSG_WAITALL、MSG_OOB 等。
        /// @return 这些调用返回接收到的字节数，如果发生错误则返回-1。在发生错误的情况下，errno会被设置以指示错误类型。
        /// 当一个流式套接字的对等方执行了有序关闭（ordered shutdown）时，返回值将为0（传统的“文件末尾”返回值）。
        /// 在不同域中的数据报套接字（例如UNIX域和Internet域）允许零长度的数据报。当接收到这样的数据报时，返回值为0。
        /// 如果从流式套接字请求接收的字节数为0，也可以返回值0。
        ssize_t recv(void *buf, size_t len,
                     int flags);

        /// @brief 从指定的套接字接收数据，并获取发送方的地址信息，多用于UDP。
        /// @param buf 指向接收数据的缓冲区的指针。
        /// @param len 缓冲区的长度。
        /// @param flags 可选的标志参数，用于指定接收操作的选项，如 MSG_WAITALL、MSG_OOB 等。
        /// @param src_addr 用于存储发送方地址信息的结构体指针。
        /// @param addrlen 指向发送方地址信息结构体长度的指针。
        /// @return 见函数 ssize_t recv(...)
        ssize_t recvfrom(void *buf, size_t len, int flags,
                         struct sockaddr *src_addr,
                         uint32_t *addrlen);

        /// @brief 通用的数据接收函数，支持流式和非流式套接字。从指定的套接字接收数据，并获取其他相关信息。
        /// @param msg 指向 struct msghdr 结构的指针，其中包含了接收数据的缓冲区指针、长度等信息。
        /// @param flags 可选的标志参数，用于指定接收操作的选项，如 MSG_WAITALL、MSG_OOB 等。
        /// @return 见函数 ssize_t recv(...)
        ssize_t recvmsg(struct msghdr *msg, int flags);

        /// @brief recvmmsg()系统调用是recvmsg()的扩展，它允许调用者使用单个系统调用从套接字接收多个消息。 
        /// （这对某些应用程序具有性能优势。）对于recvmsg()的进一步扩展是对接收操作的超时支持。
        /// @param msgvec 保存接收数据的缓冲区数组
        /// @param vlen 缓冲区数量
        /// @param flags 可选的标志参数，用于指定发送操作的选项。
        /// @param timeout 超时接受结构。
        /// @return 成功时，recvmmsg()返回在msgvec中接收到的消息数量；
        /// 发生错误时返回-1，并设置errno以指示错误原因。
        int recvmmsg(struct mmsghdr *msgvec, unsigned int vlen,
                     int flags, struct timespec *timeout);

        /// @brief 从指定的套接字发送数据，多用于TCP。
        /// @param buf 指向要发送数据的缓冲区的指针。
        /// @param len 要发送的数据的长度。
        /// @param flags 可选的标志参数，用于指定发送操作的选项，如 MSG_DONTWAIT、MSG_OOB 等。
        /// @return 成功时，这些调用返回发送的字节数。发生错误时，返回-1，并设置errno以指示错误。
        ssize_t send(void const *buf, size_t len, int flags);

        /// @brief 从指定的套接字发送数据到指定的目标地址，多用于UDP。
        /// @param buf 指向要发送数据的缓冲区的指针。
        /// @param len 要发送的数据的长度。
        /// @param flags 可选的标志参数，用于指定发送操作的选项，如 MSG_DONTWAIT、MSG_OOB 等。
        /// @param dest_addr 指向目标地址的结构体指针。
        /// @param addrlen 目标地址结构体的长度。
        /// @return  见函数 ssize_t send(...)
        ssize_t sendto(void const *buf, size_t len, int flags,
                       struct sockaddr const *dest_addr, uint32_t addrlen);

        /// @brief 通用的数据发送函数，支持流式和非流式套接字。从指定的套接字发送数据块。
        /// @param msg 指向 struct msghdr 结构的指针，其中包含了要发送的多个数据块的信息，如缓冲区指针、长度等。
        /// @param flags 可选的标志参数，用于指定发送操作的选项，如 MSG_DONTWAIT、MSG_OOB 等。
        /// @return 见函数 ssize_t send(...)
        ssize_t sendmsg(struct msghdr const *msg, int flags);

        /// @brief sendmmsg()系统调用是sendmsg(2)的扩展，
        ///  它允许调用者使用单个系统调用在套接字上传输多个消息。
        /// （这对某些应用程序具有性能优势。）
        /// @param msgvec 发送缓冲区数组（in/out）。
        /// @param vlen 缓冲区数量
        /// @param flags 可选的标志，用于指定发送选项
        /// @return 成功时，sendmmsg()返回从msgvec发送的消息数量；
        /// 如果这个值小于vlen，则调用者可以使用另一个sendmmsg()调用重试发送剩余的消息。
        /// 发生错误时，返回-1，并设置errno以指示错误原因。
        int sendmmsg(struct mmsghdr *msgvec, unsigned int vlen,
                     int flags);

        /// @brief 关闭套接字的一部分或全部连接：通知对方关闭连接或禁止继续进行数据传输。
        /// @param how SHUT_RD：关闭套接字的读取（接收）功能。
        /// SHUT_WR：关闭套接字的写入（发送）功能。
        /// SHUT_RDWR：同时关闭套接字的读取和写入功能。
        /// @return 成功时返回 0，失败时返回 -1。
        int shutdown(int how);

        /// @brief 获取原始套接字文件描述符
        /// 该函数不转移文件描述符的所有权
        /// @return int 文件描述符
        int handle()const;

        /// @brief  关闭套接字
        void close();


    private:
        int m_fd;
    };

    class LocalSocket
    {
    public:
        LocalSocket();
        LocalSocket(int domain, int type, int protocol); // int socketpair(int domain, int type, int protocol);
        ~LocalSocket();

        /// @brief 作为独立的Socket对象存在
        /// 通过该方式获得对于socket fd的操作。
        /// @param first 接受第一个套接字的Socket对象
        /// @param second 接受第二个套接字的Socket对象
        /// @return 传入的Socket对象可被成功托管，则返回true，否则false。
        bool asSocketInstance(Socket &first, Socket &second);

    private:
        int m_sv[2];
    };

    class SelectOperator
    {
    public:
        SelectOperator() = default;
        ~SelectOperator() = default;

        /// @brief 在一组文件描述符上进行 I/O 多路复用。
        /// @param nfds 要监视的最大文件描述符值加一。
        /// @param readfds 可读文件描述符集合。可以传递 NULL，表示不监视该条件。
        /// @param writefds 可写文件描述符集合。可以传递 NULL，表示不监视该条件。
        /// @param exceptfds 异常文件描述符集合。可以传递 NULL，表示不监视该条件。
        /// @param timeout 指定超时时间的结构体指针。
        /// 可以传递 NULL，表示无限期阻塞，直到有文件描述符准备好或有信号中断。
        /// @return 在发生事件或超时之时，返回准备好的文件描述符的数量；在出错时，返回 -1。
        static int select(int nfds, fd_set *readfds,
                          fd_set *writefds,
                          fd_set *exceptfds,
                          struct timeval *timeout);

        static void fdClear(int fd, fd_set *set);
        static int fdInSet(int fd, fd_set *set);
        static void fdSet(int fd, fd_set *set);
        static void fdZero(fd_set *set);

        /// @brief 在一组文件描述符上进行 I/O 多路复用。
        /// 和select相比，增加了增加了对信号屏蔽集的支持，可以选择阻塞时屏蔽特定信号。
        /// @param nfds 要监视的最大文件描述符值加一。
        /// @param readfds 可读文件描述符集合。可以传递 NULL，表示不监视该条件。
        /// @param writefds 可写文件描述符集合。可以传递 NULL，表示不监视该条件。
        /// @param exceptfds 异常文件描述符集合。可以传递 NULL，表示不监视该条件。
        /// @param timeout 指定超时时间的结构体指针。
        /// 可以传递 NULL，表示无限期阻塞，直到有文件描述符准备好或有信号中断。
        /// @param sigmask 用于指定屏蔽信号的信号集，阻塞时屏蔽指定的信号。
        /// @return 在发生事件、超时或信号中断时，返回准备好的文件描述符的数量；在出错时，返回 -1。
        static int pselect(int nfds, fd_set *readfds,
                           fd_set *writefds,
                           fd_set *exceptfds,
                           const struct timespec *timeout,
                           const sigset_t *sigmask);
    };

}
