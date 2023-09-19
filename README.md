# andren
一个库，由两部分组成，分别为base部分和network部分。但是这个区分并不很严格，因为base也可以提供网络功能。<br>
这是一个学习性质的库，目前存在着未解决的问题，以及潜在的bug，对于某些比较复杂的类，其实现是尝试性的。<br>

## 分支
暂不开dev和release分支  <br>

## 开发环境：
    Ubuntu  = Ubuntu 22.04 LTS x86_64 
    GCC = gcc version 11.3.0 (Ubuntu 11.3.0-1ubuntu1~22.04.1) 
    Make Tool = xmake v2.8.1+20230711, A cross-platform build utility based on Lua 

可以将xmake替换成cmake或者make或者其他；我觉得xmake更方便些。<br>

## 安装
脚本`install_xmake.sh`用于安装xmake构建工具。xmake的官网是`https://xmake.io/#/` 。<br>
脚本`prepare_dependent.sh`用于安装依赖到的第三方库。
脚本`update_compile_commands.sh`用于通过xmake生成`compile_commands.json`，这对在vscode里使用clangd插件很友好。
lua脚本`xmake.lua`用于生成库（.so或者.a），然后生成example。注意不要使用过多的线程编译，否则可能会爆内存。

## 其他第三方库
third-part文件夹下有三个子文件夹，对应着三个引用的库。
json库我没有直接引用仓库，而是自己下了一个版本。
llhttp需要自己生成，我自己生成了一个。
base64库理论上可以直接用OpenSSL的替换掉。

## 关于注释
    暂时没时间补完注释。总之看情况吧。

## 1. Base 
一些基本的class的集合，要求编译器不能低于C++20，但实际上可以降低到C++17甚至更低（C++11），这会使得某些代码发生一些细微变更。 <br>

### 有什么类:
    {
        File、Thread、Thread Pool、Log、GZIP File、Json、Timer、协程、Singal、Pipe、ShareMem、Socket、Epoll、编码转换、Base64、PGSQL、MYSQL。
    }

<br>上述所有的类，尤其是封装的类，都不保证提供所有的方法（如部分System call），因为实在太多了，只能等到用的时候补充。
    

### File: 
    {
        对Linux FD进行RAII封装。
    } OK

### Thread: 
    {
        对pthread的简单包装。
    } OK

### Thread Pool:
    {
        管理多个Thread对象。每一个对象一个任务队列，可对特定线程指派任务。
        基于pthread的和基于std::thread的版本都有。但我个人倾向于std::thread的版本，因为足够简单。
    } OK

### Log：
    {
        使用File提供异步日志功能。日志仅提供4个等级(Err、Warn、Debug、Info)，并打印行数。
        目前它不是ThreadLocal的，而且我也没有使用它。我打算参考学习一下其他库的LOG模块，然后再重写一个。
    } （搁置）

### Json：
    {
        直接使用nlohmann 的Json库。
    } OK

### Timer：
    {
        最小堆。 有二叉堆和四叉堆可选。精度不好说，恐怕数据不会很好看。
        这个最小堆作为定时器容器的性能有待验证，但一般来说也没人会开百万个定时器吧。
    } OK

### Signal：
    {
        只做Linux 信号量的基本包装。使用Posix信号量，包括有名信号和内存信号。
        用以提供进程或线程间通信。
        至于SystemV信号量，kill和signal这两个函数，直接用就是。
    } OK

### Pipe：
    {
        只做Linux 有名管道的基本包装。使用Linux提供的管道，包括匿名和具名管道。
        用以提供进程或线程间通信。
    } OK

### ShareMem：
    {
        只做Linux共享内存的基本包装。使用Posix共享内存，且具名。
    } OK

### 进程锁：
    {
        使用pthread_mutex达成。依赖Posix共享内存。
        不管怎么说，比用文件锁舒服，毕竟文件锁有着其他更好的用途。
    } OK
    
### Socket：
    {
        只做Socket的基本包装。不保证提供所有操作方法。
        Socket的东西比较繁杂，所以我尝试做一个UDP的Class和TCP Client/Server的Class。
        但是因为在实际编写Socket示例时发现，即使没有这两个类也能工作的很好，
        更进一步的封装类在network部分。
        OpenSSL Socket IO部分的example已经整好,我是用的自签名证书测的，实际用需要替换。
    } OK
    
### Epoll：
    {
        只做Epoll接口的基本包装。
        在Socket示例种使用到。
        network部分的RePoller会依赖它。
    } OK

### GZip Stream：
    {
        用使用gzip库和archive库，提供数据流压缩能力，文件压缩解压能力（7zip）。
        文件压缩解压缩用的时7zip。这个东西可能会很慢，有空的话再加一个zip模式的吧。
    } OK

### 协程 Coroutine：
    {
        使用C++20的协程做。
        C++20的协程我反复学了好几次，目前只能写出简单的协程Demo。
        我认为这是自己对协程和协程的使用场景缺乏了解的缘故，因此暂时先搁置下来。
    }（搁置）

### 编码转换TextCodec:
    {
        UTF8、UTF16、UTF32、GBK编码互转支持。
        涉及到GBK就会麻烦起来。
    }（搁置）

### 编码转换BASE64：
    {
        依赖base64库。
        出处： https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
        其实OpenSSL也带有BASE64的编码解码支持，因此这个库随时可以替换成使用OpenSSL的。
    } OK

### PGSQL：
    {
        PostgreSQL C API 简单包装。不保证提供所有接口。
        普通SQL，预处理SQL，同步和异步Query。
        PGSQL 并没有MYSQL那么易用，但libpq的体验确实非常好。
    } OK

### MYSQL：
    {
        MYSQL C API 简单包装。不保证提供所有接口。(Statement)
        预处理部分的接口实在不好包装，因为某些数据的类型或者长度我没有方便的方法拿到，这要求用户预先知道类型或者长度。
    } OK 


## 2. Network Classes 集合
    指的是提供网络功能的类的集合。

### 有什么类:
    {
        NGHTTP、HiRedis、TCP、UDP、IO_Uring、Event Loop ，FreelockShareMemory ， TaskDistributor 及其相关的类
    }

### TCP/UDP：
    {
        提供TCP Stream ， TCP Stream Manager ， User Datagram。
        UDP和TCP大部分基本写好了，但是没有支持IPV6。
        目前有两个版本：{
            版本1：{
                task_distributor.cpp + repoller.cpp + sockstream.cpp + acceptor.cpp + datagram.cpp 及其各自的.hh头文件。
                这个版本使用任务分派和epoll的poller。
            }，
            版本2：{
                task_distributor.cpp + executor.cpp + schedule.cpp + events_service.cpp + eventloop.cpp及其各自头文件。
                这个版本使用eventloop 、eventsservice、executor等设施，在iouring等的基础下下实现异步。
                具体的异步方式有继承了eventservice的类实现，比如iouring的io完成通知在io_service.cpp的IOService中提供。
            }
        }
    }（OK）

### io_uring：
    {
        使用liburing ，而不是直接使用系统调用。
        io_uring在此主要是处理网络io的fd和文件io的fd。
        目前仅仅完成了Socket FD和文件FD的最基本的支持，暂未测试。
        io_uring实例存在于IOService类中。
    } （OK）

### TaskDistributor：
    {
        TaskDistributor 每次循环按顺序处理各类事件：{
            1. TaskDistributor 基于ThreadPool 和 DBuffer（或其他队列）。（强制）各线程依照顺序标记为1 ~ n，线程池中线程数量不能少于2个。
            2. 定时器任务（仅线程2）。 （约定）截取且仅截取一次当前时间，并取出所有超时任务进行执行。
            3. 网络IO任务（仅线程1）。 （约定）处理网络断开事件，新连接事件、可读事件、可写事件。断开的网络在此一次性清理。
            4. 文件IO任务（仅线程1）。 （约定）处理IO可读可写事件。
            5. 普通任务（线程1 ~ n）。 处理任务队列里的任务。
            6.自定义分配。允许手动指定要运行task的线程。
        }
        根据具体的情况，该部分可能会存在增删。
        第四项，因为Linux的异步IO不好整，目前用线程池做的模拟。
    } （OK）

### EventLoop：
    { 
        Eventloop支持以下事件：
            1.定时器超时事件集。（EPOLL\IO_URING\用户态定时器）
            2.网络异步IO事件集。（IO URING）
            3.文件异步IO事件集。（IO URING）
            4.数据库IO异步事件集。（由数据库客户端库的封装类提供事件）
            5.特定事件集。（由用户手动控制其是否发生）

        EventLoop依赖TaskDistributor（自定义分配），因为事件的分发需要使用到任务分发机制。
        EventLoop对所有的事件集，都使用统一的int64_t类型的key作为事件fd，
        EventLoop支持上诉事件集，是由EventsService提供的，
        目前实现了IOService（继承自EventsService，使用liburing）。

        因为每个线程都有一个EventsService表，因此理论上每种EventsService都能在
        每个线程中创建一个各自不同的实例，以更好地利用多核处理器的性能。

        通常使用Schedule类可以自动分配具体的功能到某个线程对应的Service，其分配结果是稳定的。

    }（计划中）

### RePoller：
    {
        使用base部分的EPOLL封装，并且结合 TaskDistributor ，做到事件分发。
        支持手动发送事件（如果你觉得Epoll Wait不出来）。同时也支持非Epoll的fd（即FD <=-2 ，该FD不被EPOLL_CTL_ADD），用于支持可控触发。
        对于文件或者自定义的FD，可以对其取负值，这样RePoller不会将之注册监听到EPOLL。
        此RePoller不对Channel服务，也没有提供一个Channel抽象，而是做进一步的事件分派，并帮助FD Provider接入 TaskDistributor 。
        具体怎么去处理这个事件，是FD Provider的事情。
        RePoller还附带有一个简单的对象生命周期托管功能。
    } （OK）

### GRPC ：
    {
        GRPC有C++库,已经足够好用了，也就无需包装。
        编写一个example示例。
        后续考虑怎么融入事件循环中。
    } （OK）

### HTTP：
    {
        使用nghttp2，nghttp3以支持http2/3。
        其实nghttp2也支持http3.0。但nghttp3却是专门用于支持http3.0。
        nghttp2支持http1.1和http2.0。
        这个部分目前仍在开发，已完成的功能是接收Request；因为我对nghttp2的理解还不够，导致目前进度比较缓慢。
    } （正在） 

### Redis：
    {
        Redis客户端 c api。以hiredis为基础，只做基本的包装。
        可替代的客户端库很多，比如ACL的，体验就不错。
    } （计划）

### FreeLock SharedMemory ：
    {
        使用内存屏障在共享内存的基础上，把锁去掉了。
        目前它的性能，在虚拟机上测出来比较弱，而且还有内存拷贝的花销没解决。
        未来打算弄一个环形队列，队列的每一个节点都是一个动态大小的缓冲区。
    }（OK）
