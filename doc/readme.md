#来自Chrndex Family‘s Library的Andren库

Chrndex Family‘s Library指Chrindex系列的库，通常使用C/C++编写，但也可能以其他语言的形式提供。

Chrindex 系列库包含多个库，以下为他们的代号：

1. [Andren] (安卷) 基础库、网络库、协程库部分功能整合。 

2. [Kinser] (镜法)计算库、数据库连接池。OpenCV、OpenCL库的部分功能整合、PostgreSql、Mysql、SQLServer的Client SDK功能的部分整合。

3. [MengLenxy]  简单GUI库、LUA虚拟机库。 Vulkan库功能整合、LUA功能的部分整合。

4. [Lanyan] 流媒体库。FFMPEG + [Andren] + RSTP。


Andren库为Chrindex系列库的网络库，当前版本为0.1.001.L-dev

当前版本仅支持Linux。
版本号说明：主版本号.子版本号.修补序号.系统支援-状态描述

Andren库具有以下特性：
1. Base Classes 集合
    指的是一些基本类库的集合，要求编译器不能低于C++20。
    一般有File库、Thread库、Thread Pool库、Log库、GZIP File库、Json、
    Timer库、协程库、Singal库、Pipe库、ShareMem库。

    File库: {
        对Linux FD进行RAII封装， 不保证提供所有IO操作方法。
    } OK

    Thread库: {
        对pthread的简单包装。不一定提供所有的pthread操作方法。
    } OK

    Thread Pool库:{
        管理多个Thread对象。
    } OK

    Log库：{
        使用File提供异步日志功能。日志仅提供4个等级(Err、Warn、Debug、Info)，并打印行数。
        可选使用GZip压缩Log文件。
    } OK

    GZip File库：{
        用GNU的库，提供一些压缩能力。至少Log库会用到。
    } 

    Json库：{
        使用nlohmann 的Json库。
    } OK

    Timer库：{
        Loop Tick + 时间轮， 类似Libuv的做法。 
    } OK

    协程库：{
        使用C++20的协程做。目前优先级排在最后。
    } 

    Signal库：{
        只做Linux 信号量的基本包装。
    } 

    Pipe库：{
        只做Linux 有名管道的基本包装。
    } 

    ShareMem库：{
        只做Linux共享内存库的基本包装。
        提供进程锁（由pthread_mutex升格）。
    } 


2. Network Classes 集合
    指的是网络库的集合。
    一般有Socket、TCP、UDP、IPv4 End Point、IO_Uring、Epoll、Event Loop、
    HiRedis库。
    
    Socket库：{
        只做Socket的基本包装。不保证提供所有操作方法。
    }

    TCP/UDP库：{
        只做两者的基本包装。提供TCP Server ， TCP Client ， UDP Stream Manager。
    }

    IPV4 EndPoint库：{
        提供在网络序和主机序之间的IPV4表示和转换。
    }

    io_uring库：{
        只做Linux的io_uring的基本包装。
    }

    Epoll库：{
        只做Epoll接口的基本包装。
    }

    EventLoop库：{
        Eventloop每次循环按顺序处理各类事件：{
            0. EventLoop基于ThreadPool 。各线程依照顺序标记为1~n
            1. 定时器事件（仅线程3~n）。 截取且仅截取一次当前时间，并取出所有超时任务进行执行。
            2. 网络IO任务（仅线程1）。 处理网络断开事件，新连接事件、可读事件、可写事件。断开的网络在此一次性清理。
            3. 文件IO事件（仅线程2）。 处理IO可读可写事件。
            4. 普通任务事件（仅线程4~n）。 处理任务队列里的任务。
            5. 当且仅当线程无事可做时等待1MS（实际可能会更久）。
            6. 处理不同任务的线程使用不同的线程函数。
        }
    }

    HTTP库：{
        使用llhttp库。只做基本包装。
    } 

    Redis库：{
        Redis Client SDK 库。使用hiredis为基础，只做基本的包装。
    }