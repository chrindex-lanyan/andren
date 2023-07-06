# andren
一个库，由两部分组成，分别为base部分和network部分。

1. Base 
    一些基本的class的集合，要求编译器不能低于C++20。
    有File库、Thread库、Thread Pool库、Log库、GZIP File库、Json、
    Timer库、协程库、Singal库、Pipe库、ShareMem库。

    File库: {
        对Linux FD进行RAII封装， 不保证提供所有IO操作方法。
    } OK

    Thread库: {
        对pthread的简单包装。不一定提供所有的pthread操作方法。
    } OK

    Thread Pool库:{
        管理多个Thread对象。每一个对象一个任务队列，可对特定线程指派任务。
    } OK

    Log库：{
        使用File提供异步日志功能。日志仅提供4个等级(Err、Warn、Debug、Info)，并打印行数。
    } OK

    Json库：{
        不造轮子了，直接使用nlohmann 的Json库。
    } OK

    Timer库：{
        Loop Tick + 最小堆。 有二叉堆和四叉堆可选。
    } OK

    Signal库：{
        只做Linux 信号量的基本包装。使用Posix信号量，包括有名信号和内存信号。
        用以提供进程或线程间通信。
    } OK

    Pipe库：{
        只做Linux 有名管道的基本包装。使用Linux提供的管道，包括匿名和具名管道。
        用以提供进程或线程间通信。
    } OK

    ShareMem库：{
        只做Linux共享内存的基本包装。使用Posix共享内存，且具名。
    } 

    进程锁：{
      使用pthread_mutex达成。
    } 

    GZip File库：{
        用GNU的库，提供一些压缩能力。至少Log库会用到。
    } （暂时先不实现）

    协程库：{
        使用C++20的协程做。
    } （比较费时间，暂时先不填坑）


3. Network Classes 集合
    指的是提供网络功能的类的集合。
    有Socket、TCP、UDP、IPv4 End Point、IO_Uring、Epoll、Event Loop、
    HiRedis相关的类。
    
    Socket库：{
        只做Socket的基本包装。不保证提供所有操作方法。
    }

    TCP/UDP库：{
        只做Socket的基本包装。提供TCP Stream ， TCP Stream Manager ， UDP Stream Manager。
        Accept、Read、Write是可非阻塞的。
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
            1. EventLoop基于ThreadPool 。各线程依照顺序标记为1 ~ n，线程池中线程数量不能少于两个。
            2. 定时器任务（线程1 ~ n）。 截取且仅截取一次当前时间，并取出所有超时任务进行执行。
            3. 网络IO任务（仅线程1）。 处理网络断开事件，新连接事件、可读事件、可写事件。断开的网络在此一次性清理。
            4. 文件IO任务（仅线程1）。 处理IO可读可写事件。
            5. 普通任务（线程1 ~ n）。 处理任务队列里的任务。
            6. 当且仅当线程无事可做时等待1MS（实际可能会更久）。
            7. 定时器任务、普通任务优先选择其他线程，然后才选择线程1。
        }
    }

    HTTP库：{
        使用llhttp库。只做基本包装。
    } 

    Redis库：{
        Redis 客户端。以hiredis为基础，只做基本的包装。
    }
