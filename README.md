# andren
一个库，由两部分组成，分别为base部分和network部分。但是这个区分并不很严格，因为base也可以提供网络功能。<br>
这是一个学习性质的库，目前存在许多理解和实践上的问题。<br>
你当然可以指出问题，但我可能会在其他地方改进，因为我很可能在未来的某个时候将这个库推翻重写。<br>

开发环境：<br>
    Ubuntu  = Ubuntu 22.04 LTS x86_64 <br>
    GCC = gcc version 11.3.0 (Ubuntu 11.3.0-1ubuntu1~22.04.1) <br>
    Make Tool = xmake v2.8.1+20230711, A cross-platform build utility based on Lua <br>

当然可以将xmake替换成cmake或者make或者其他；除非不得己，我真的不想写cmake。<br>

## 关于注释
    暂时没时间补完注释。总之看情况吧。

## 1. Base 
    一些基本的class的集合，要求编译器不能低于C++20，但实际上可以降低到C++17甚至更低（C++11），这仅仅会使得某些局部的代码可能会变得啰嗦一点。 

    Base部分有File库、Thread库、Thread Pool库、Log库、GZIP File库、Json、Timer库、协程库、Singal库、Pipe库、ShareMem库、Socket库、Epoll库、编码转换、Base64编码、PGSQL库、MYSQL库、LLHTTP库、HiRedis。 

    上述所有的类库，尤其是封装的类，都不保证提供所有的方法。
    

### File库: 
    {
        对Linux FD进行RAII封装。
    } OK

### Thread库: 
    {
        对pthread的简单包装。
    } OK

### Thread Pool库:
    {
        管理多个Thread对象。每一个对象一个任务队列，可对特定线程指派任务。
    } OK

### Log库：
    {
        使用File提供异步日志功能。日志仅提供4个等级(Err、Warn、Debug、Info)，并打印行数。
        目前这玩意实现的比较简陋，可能以后会改成Thread Local的。
    } OK

### Json库：
    {
        造这种轮子没啥意思，直接使用nlohmann 的Json库。
    } OK

### Timer库：
    {
        最小堆。 有二叉堆和四叉堆可选。
        这个最小堆作为定时器容器的性能有待验证。
    } OK

### Signal库：
    {
        只做Linux 信号量的基本包装。使用Posix信号量，包括有名信号和内存信号。
        用以提供进程或线程间通信。
        至于SystemV信号量，kill和signal这两个函数，直接用就是。
    } OK

### Pipe库：
    {
        只做Linux 有名管道的基本包装。使用Linux提供的管道，包括匿名和具名管道。
        用以提供进程或线程间通信。
    } OK

### ShareMem库：
    {
        只做Linux共享内存的基本包装。使用Posix共享内存，且具名。
    } OK

### 进程锁：
    {
      使用pthread_mutex达成。依赖Posix共享内存。
    } OK
    
### Socket库：
    {
        只做Socket的基本包装。不保证提供所有操作方法。
        Socket的东西比较繁杂，所以我尝试做一个UDP的Class和TCP Client/Server的Class。
        但是因为在实际编写Socket示例时发现，即使没有这两个类也能工作的很好，
        因此我觉得暂时将实现UDP的Class和TCP Client/Server的Class的优先级降低。

        OpenSSL Socket IO部分的example已经整好,我是用的自签名证书测的，实际用需要替换。

    } OK
    
### Epoll库：
    {
        只做Epoll接口的基本包装。
        在Socket示例种使用到。
    } OK

### GZip Stream库：
    {
        用使用gzip库和archive库，提供数据流压缩能力，文件压缩解压能力（7zip）。
        文件压缩解压缩用的时7zip。这个东西可能会很慢，有空的话再加一个zip模式的吧。
    } OK

### 协程库 Coroutine：
    {
        使用C++20的协程做。
        C++20的协程我反复学了好几次，目前只能写出简单的协程Demo。
        我认为这是自己对协程和协程的使用场景缺乏了解的缘故，因此暂时先搁置下来。
    }（比较费时间，暂时先不填坑）

### 编码转换库TextCodec:
    {
        UTF8、UTF16、UTF32、GBK编码互转支持。
        涉及到GBK就会麻烦起来。
    }（目前没有找到比较好的第三方库，暂时先不整）

### 编码转换BASE64库：
    {
        依赖base64库。
        出处： https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
        其实OpenSSL也带有BASE64的编码解码支持，因此这个库随时可以替换成使用OpenSSL的。
    } OK

### HTTP库：
    {
        使用nghttp2，nghttp3以支持http2/3。
        其实nghttp2也支持http3.0。但nghttp3却是专门用于支持http3.0。
        nghttp2支持http1.1和http2.0。但是这个库的接口较为繁琐，这并不是nghttp2的锅，
        因为http2.0是二进制协议，且它引入了stream和frame的概念之后，状态的处理变得复杂起来，和http1.0完全就不是一个东西了。
        在我对http还是不够了解的情况下，这种复杂更甚。我可能会通过其他方式由浅入深地去重新认识http。
    } （停滞）

### Redis库：
    {
        Redis客户端 c api。以hiredis为基础，只做基本的包装。
        由于时间的问题，这部分暂时搁置。
        可替代的客户端库很多，比如ACL的，体验就不错。
    } （停滞）

### PGSQL库：
    {
        PostgreSQL C API 简单包装。不保证提供所有接口。
        普通SQL，预处理SQL，同步和异步Query。
        PGSQL 的体验非常良好，即使我已经意识到它并没有MYSQL那么易用，
        但libpq客户端库的体验仍然非常良好。
    } OK

### MYSQL库：
    {
        MYSQL C API 简单包装。不保证提供所有接口。(Statement)
        预处理部分的接口实在不好包装。
    } OK 


## 2. Network Classes 集合
    指的是提供网络功能的类的集合。
    有TCP、UDP、IO_Uring、Event Loop相关的类。
    
### TCP/UDP库：
    {
        只做Socket的基本包装。提供TCP Stream ， TCP Stream Manager ， UDP Stream Manager。
        Accept、Read、Write是可非阻塞的。
    } (暂时搁置，具体原因见`Socket库`部分)

### io_uring库：
    {
        只做Linux的io_uring的基本包装。
    } （暂时搁置）

### EventLoop库：
    {
        Eventloop每次循环按顺序处理各类事件：{
            1. EventLoop基于ThreadPool 。各线程依照顺序标记为1 ~ n，线程池中线程数量不能少于1个。
            2. 定时器任务（线程1 ~ n）。 截取且仅截取一次当前时间，并取出所有超时任务进行执行。
            3. 网络IO任务（仅线程1）。 处理网络断开事件，新连接事件、可读事件、可写事件。断开的网络在此一次性清理。
            4. 文件IO任务（仅线程1）。 处理IO可读可写事件。
            5. 普通任务（线程1 ~ n）。 处理任务队列里的任务。
            6. 当且仅当线程无事可做时等待1MS（实际可能会更久）。
            7. 定时器任务、普通任务优先选择其他线程，然后才选择线程1。
        }
        根据具体的情况，该部分可能会存在增删。
    } （正在）

### GRPC 库：
    {
        GRPC的C++库，因此不做包装。
        只编写example示例。
    } （正在）

    

