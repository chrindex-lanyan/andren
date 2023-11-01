

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <fmt/core.h>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <iostream>
#include <utility>

#include "acceptor.hh"
#include "sockstream.hh"
#include "end_point.h"
#include "freelock_smem.hh"


using namespace chrindex::andren;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit;

std::atomic<uint64_t> m_uid = 1;

std::string serverip = "192.168.88.3";
int32_t serverport = 8317;
std::shared_ptr<network::TaskDistributor> eventLoop;
std::shared_ptr<network::RePoller> repoller;
std::shared_ptr<network::Acceptor> acceptor;
base::Socket ssock(AF_INET, SOCK_STREAM,0);
base::EndPointIPV4 epv4(serverip,serverport);


std::string shmem_name_wr = "/shmemory_WRITE";
std::string shmem_name_rd = "/shmemory_READ";
network::FreeLockShareMemWriter writer(shmem_name_wr, true);
network::FreeLockShareMemReader reader(shmem_name_rd, true);

static constexpr uint64_t default_magic_01 = 0xa9b8c7d6e5f4321c;
static constexpr uint64_t default_magic_02 = 0xf13bc45df678ea9e;


enum class OperatorEnum: uint16_t
{
    NONE = 0,

    NOTIFY_NEW_CLIENT,
    NOTIFY_CLIENT_CLOSED,
    NOTIFY_CLIENT_ON_READ,
    NOTIFY_CLIENT_ON_WRITE,

    REQUEST_WRITE_CLIENT,
    REQUEST_CLOSE_CLIENT,

};

struct TransSendDataHead
{
    //// 128 bit magic
    uint64_t magic_01;
    uint64_t magic_02;
    
    /// message uid
    uint64_t message_uid;
    
    /// message frame uid
    uint64_t frame_uid;

    /// frame size
    uint16_t frame_size;
    
    ///  operator
    uint16_t op;
};

struct TransSendData
{
    /// head
    TransSendDataHead head;

    /// data
    union _Data_U
    {
        int64_t i64_val;
        uint64_t u64_val;
        sockaddr_storage ss;
        char buffer[4096 - sizeof(head)];
    } data;
};


int freelock_sharedmem()
{
    assert(writer.valid());
    assert(reader.valid());

    return 0;
}


int sendToSharedMemory(TransSendData const & data)
{
    if (writer.writable())
    {
        writer.write_some(std::string( reinterpret_cast<char const *>(&data)  , sizeof(data))  );
    }
    else 
    {
        eventLoop->addTask([data]()
        {
            sendToSharedMemory(data);
        },network::TaskDistributorTaskType::IO_TASK);
    }

    return 0;
}

int dealwith_read(std::string && data);

int readFromSharedMemory()
{
    if (reader.readable())
    {
        std::string data;
        reader.read_some(data);
        dealwith_read(std::move(data));
    }
    
    eventLoop->addTask([]()
    {
        readFromSharedMemory();
    },network::TaskDistributorTaskType::IO_TASK);
    
    return 0;
}


int dealwith_read(std::string && data)
{
    
    return 0;
}


int tcpServer()
{
    int ret ;
    bool bret;

    eventLoop = std::make_shared<network::TaskDistributor>(4); // 4个线程

    // 开始事件循环
    bret = eventLoop->start();
    assert(bret);

    repoller = std:: make_shared<network::RePoller>();
    acceptor = std::make_shared<network::Acceptor>(eventLoop,repoller);

    int optval = 1;
    ret = ssock.setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    assert(ret ==0);

    ret = ssock.bind(epv4.toAddr(),epv4.addrSize());
    assert(ret == 0);

    ret = ssock.listen(100);
    assert(ret == 0);

    genout("TCP Server : Start Listen And Prepare Accept...\n");

    acceptor->setOnAccept([ ](std::shared_ptr<network::SockStream> link)
    {
        if(!link) 
        {
            m_exit= 1;
            errout("TCP Server : Accept Faild! Socket Error.\n");
            return ;
        }
        int fd = link->reference_handle()->handle();
        std::weak_ptr<network::SockStream> wlink = link;
        std::weak_ptr<network::RePoller> wrepoller = link->reference_repoller();
        std::shared_ptr<network::RePoller> repoller = wrepoller.lock();

        link->setOnClose([wrepoller , fd, wlink]() 
        {
            errout("TCP Server : Client Disconnected .\n");
            std::shared_ptr<network::RePoller> repoller = wrepoller.lock();
            if (!repoller)
            {
                return ;
            }

            /// 将link从对象池里去除
            repoller->findObject(fd, true , [ ](bool ret, std::any * obj)
            {
                genout("TCP Server : This Client Is TakeOut From The ObjectsMap.\n");
            });

            sockaddr_storage ss;
            uint32_t len = sizeof(sockaddr_storage);
            wlink.lock()->reference_handle()->getpeername(reinterpret_cast<sockaddr*>(&ss), &len);

            TransSendData tsd;

            tsd.head.magic_01 = default_magic_01;
            tsd.head.magic_02 = default_magic_02;
            tsd.head.message_uid = m_uid++;
            tsd.head.frame_uid=0;
            tsd.head.frame_size = sizeof(ss);
            tsd.head.op = static_cast<uint16_t>(OperatorEnum::NOTIFY_CLIENT_CLOSED);
            tsd.data.ss = ss;

            sendToSharedMemory(tsd);

            return ;
        });

        link->setOnWrite([ ](ssize_t ret, std::string && data) 
        {
            genout("TCP Server : Write Some Data To Client Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
            if(data.size() != ret)
            {
                TransSendData tsd;

                tsd.head.magic_01 = default_magic_01;
                tsd.head.magic_02 = default_magic_02;
                tsd.head.message_uid = m_uid++;
                tsd.head.frame_uid=0;
                tsd.head.frame_size = sizeof(ret);
                tsd.head.op = static_cast<uint16_t>(OperatorEnum::NOTIFY_CLIENT_ON_WRITE);
                tsd.data.i64_val = ret;

                sendToSharedMemory(tsd);
            }
        });

        // weak防止循环引用
        link->setOnRead([wlink](ssize_t ret, std::string && data) mutable
        {
            if (ret < 200) // 太大则不打印内容
            {
                genout("TCP Server : Read Some Data From Client ...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
            }else 
            {
                genout("TCP Server : Read Some Data From Client ...Size = [%ld].\t\r",ret);
            }
            auto link =wlink.lock();
            if(!link)
            {
                return ;
            }

            auto ev = link->reference_repoller().lock()->eventLoopReference().lock();

            sockaddr_storage ss;
            uint32_t len = sizeof(sockaddr_storage);
            link->reference_handle()->getpeername(reinterpret_cast<sockaddr*>(&ss), &len);

            TransSendData tsd;
            uint64_t frameid = 0;

            tsd.head.magic_01 = default_magic_01;
            tsd.head.magic_02 = default_magic_02;

            if(data.size() > (sizeof(tsd.data.buffer) + sizeof(tsd.data.ss)))
            {
                tsd.head.message_uid = m_uid++;
                tsd.head.frame_uid=frameid++;
                tsd.head.frame_size = sizeof(ss) + data.size();
                tsd.head.op = static_cast<uint16_t>(OperatorEnum::NOTIFY_CLIENT_ON_READ);
                tsd.data.ss = ss;

                size_t pos = sizeof(tsd.data.buffer) - sizeof(tsd.data.ss);

                ::memcpy(&tsd.data.buffer[sizeof(ss)],&data[0], 
                    pos); 

                ev->addTask([tsd]() mutable
                {
                    ::sendToSharedMemory(tsd);
                },network::TaskDistributorTaskType::IO_TASK);

               
                for (size_t index = pos 
                    ; index < data.size()-1 ; )
                {
                    uint64_t crrtsize = std::min( sizeof(tsd.data.buffer) , data.size() - index );

                    tsd.head.message_uid = m_uid++;
                    tsd.head.frame_uid= frameid++;
                    tsd.head.frame_size = crrtsize;
                    tsd.head.op = static_cast<uint16_t>(OperatorEnum::NOTIFY_CLIENT_ON_READ);

                    ::memcpy(&data[index], tsd.data.buffer, crrtsize); 

                    index += crrtsize;

                    ev->addTask([tsd]() mutable
                    {
                        ::sendToSharedMemory(tsd);
                    },network::TaskDistributorTaskType::IO_TASK);
                }
            }
            else 
            {
                tsd.head.message_uid = m_uid++;
                tsd.head.frame_uid=0;
                tsd.head.frame_size = sizeof(ss) + data.size();
                tsd.head.op = static_cast<uint16_t>(OperatorEnum::NOTIFY_CLIENT_ON_READ);
                tsd.data.ss = ss;

                ::memcpy(&tsd.data.buffer[sizeof(ss)],&data[0], data.size()); 

                ev->addTask([tsd]() mutable
                {
                    ::sendToSharedMemory(tsd);
                },network::TaskDistributorTaskType::IO_TASK);
            }

        });

        /// 监听读取事件
        bool bret = link->startListenReadEvent();
        assert(bret);

        /// 强引用保存到对象MAP
        /// 反正用到的时候可以查
        bret = repoller->saveObject(fd, true, link, [](bool ret, std::any * obj)
        {
            genout("TCP Server : This Client Is Saved Into The ObjectsMap.\n");
        });
        assert(bret);

        sockaddr_storage ss;
        uint32_t len = sizeof(sockaddr_storage);
        link->reference_handle()->getpeername(reinterpret_cast<sockaddr*>(&ss), &len);

        TransSendData tsd;

        tsd.head.magic_01 = default_magic_01;
        tsd.head.magic_02 = default_magic_02;
        tsd.head.message_uid = m_uid++;
        tsd.head.frame_uid=0;
        tsd.head.frame_size = len;
        tsd.head.op = static_cast<uint16_t>(OperatorEnum::NOTIFY_NEW_CLIENT);
        tsd.data.ss = ss;

        sendToSharedMemory(tsd);
    });

    // 开始polling
    bret = repoller->start(eventLoop,2);
    assert(bret);

    // 开始accept
    bret = acceptor->start(std::move(ssock));
    assert(bret);

    eventLoop->addTask([]()
    {
        readFromSharedMemory();
    },network::TaskDistributorTaskType::IO_TASK);

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    genout ("TCP Server : Server Exit...\n");
    return 0;
}


int read_config()
{
    m_exit = 0;
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   genout("准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        errout("Cannot registering signal handler.\n");
        return -3;
    }

    genout("TCP Server Config : %s.\n", 
    fmt::format("Address [{}:{}] ;\n SharedMemory read::[{}] ;\n write::[{}]"
                    " ; Magic01 [{}] ;\nMagic02 [{}] .\n", 
        serverip, serverport , shmem_name_rd, shmem_name_wr,default_magic_01,default_magic_02 )
    .c_str());

    freelock_sharedmem();

    tcpServer();

    return 0;
}



int main(int argc , char ** argv)
{
    read_config();
    return 0;
}



