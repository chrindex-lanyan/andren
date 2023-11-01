

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <fmt/core.h>
#include <future>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <iostream>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include "socket.hh"
#include "task_distributor.hh"
#include "repoller.hh"
#include "acceptor.hh"
#include "memclear.hpp"

#include "datagram.hh"


using namespace chrindex::andren;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit;

std::string domainName = "MyUnixSocket";


int testUnixDomainStreamServer()
{
    std::shared_ptr<network::TaskDistributor> eventLoop;
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::Acceptor> acceptor;
    base::Socket ssock(AF_UNIX, SOCK_STREAM,0);
    int ret ;
    bool bret;

    eventLoop = std::make_shared<network::TaskDistributor>(4); // 4个线程

    // 开始事件循环
    bret = eventLoop->start();
    assert(bret);

    repoller = std:: make_shared<network::RePoller>();
    acceptor = std::make_shared<network::Acceptor>(eventLoop,repoller);

    // 开始polling
    bret = repoller->start(eventLoop,10);
    assert(bret);

    // 地址重用
    int optval = 1;
    ret = ssock.setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    assert(ret ==0);

    // 删除旧的
    ret = ::system(fmt::format("rm {}", domainName).c_str());

    // 绑定文件
    sockaddr_un saddr;
    base::ZeroMemRef(saddr);
    saddr.sun_family = AF_UNIX;
    ::memcpy(saddr.sun_path,domainName.c_str(),
         std::min(sizeof(saddr.sun_path), domainName.size()) );
    size_t size = sizeof(saddr.sun_family) + 1 +  domainName.size();
    ret = ssock.bind(reinterpret_cast<sockaddr*>(&saddr) ,  size);
    assert(ret == 0);

    // 监听
    ret = ssock.listen(100);
    assert(ret == 0);

    // 设置回调
    acceptor->setOnAccept([ ](std::shared_ptr<network::SockStream> link)
    {
        genout("Stream Server : Accepted Once , Processing...\n");
        if(!link) 
        {
            m_exit= 1;
            errout("Stream Server : Accept Faild! Socket Error.\n");
            return ;
        }
        int fd = link->reference_handle()->handle();
        std::weak_ptr<network::SockStream> wlink = link;
        std::weak_ptr<network::RePoller> wrepoller = link->reference_repoller();
        std::shared_ptr<network::RePoller> repoller = wrepoller.lock();

        link->setOnClose([wrepoller , fd]() 
        {
            errout("Stream Server : Client Disconnected .\n");
            std::shared_ptr<network::RePoller> repoller = wrepoller.lock();
            if (!repoller)
            {
                return ;
            }

            /// 将link从对象池里去除
            repoller->findObject(fd, true , [ ](bool ret, std::any * obj)
            {
                genout("Stream Server : This Client Is TakeOut From The ObjectsMap.\n");
            });
            return ;
        });

        link->setOnWrite([ ](ssize_t ret, std::string && data) 
        {
            genout("Stream Server : Write Some Data To Client Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
            if(ret == -1)
            {
                genout("SUB UnixDomain DG : errstr = %m.\n",errno);
            }
        });

        // weak防止循环引用
        link->setOnRead([wlink](ssize_t ret, std::string && data) mutable
        {
            if (ret < 200) // 太大则不打印内容
            {
                genout("Stream Server : Read Some Data From Client ...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
            }else 
            {
                genout("Stream Server : Read Some Data From Client ...Size = [%ld].\t\r",ret);
            }
        });

        /// 监听读取事件
        bool bret = link->startListenReadEvent();
        assert(bret);

        /// 强引用保存到对象MAP
        /// 反正用到的时候可以查
        bret = repoller->saveObject(fd, true, link, [](bool ret, std::any * obj)
        {
            genout("Stream Server : This Client Is Saved Into The ObjectsMap.\n");
        });
        assert(bret);
    });

    genout("Stream Server : Start Listen And Prepare Accept...\n");

    // 开始accept
    bret = acceptor->start(std::move(ssock));
    assert(bret);

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    genout ("Stream Server : Server Exit...\n");
    return 0;
}

int testUnixDomainStreamClient()
{
    std::shared_ptr<network::TaskDistributor> eventLoop;
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::SockStream> link;
    base::Socket csock(AF_UNIX, SOCK_STREAM,0);
    bool bret;

    eventLoop = std::make_shared<network::TaskDistributor>(4); // 4个线程

    // 开始事件循环
    bret = eventLoop->start();
    assert(bret);

    repoller = std::make_shared<network::RePoller>();
    link = std::make_shared<network::SockStream>(std::move(csock),repoller);

    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    std::weak_ptr<network::SockStream> wlink = link;

    genout("Stream Client : Prepare To  Connect Server...\n");

    link->setOnClose([]()
    {
        m_exit= 1;
        errout("Stream Client : Disconnected , Prepare Exit...\n");
    });

    link->setOnWrite([](ssize_t ret, std::string && data)
    {
        if (ret < 200)
        {
            genout("Stream Client : Write Some Data To Server Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("Stream Client : Write Some Data To Server Done...Size = [%ld]\t\r.",ret);
        }
        if(ret == -1)
        {
            genout("SUB UnixDomain DG : errstr = %m.\n",errno);
        }
    });

    link->setOnRead([wlink](ssize_t ret, std::string && data)mutable
    {
        if (ret < 200)
        {
            genout("Stream Client : Read Some Data From Server, EchoBack ...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("Stream Client : Read Some Data From Server, EchoBack ...Size = [%ld]\t\r.",ret);
        }
    });

    // 开始polling
    bret = repoller->start(eventLoop,50);
    assert(bret);

    /// 注意，要先polling才能asyncConnect
    bret = link->asyncConnect(domainName, [link, &promise](bool ok)
    {
        genout("Stream Client : OnConnected = %s.\n",(ok?"True":"False"));
        promise.set_value(ok);
    });
    assert(bret);

    while(m_exit != 1)
    {
        if (future.valid())
        {
            bret = future.get();
            break;
        }else 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (!bret)
    {
        errout("Stream Client : Connect Failed...Exit...\n");
        m_exit=1;
    }
    else 
    {
        for (int i =0; i< 5 ; i++)
        {
            bret = link->asend("hello world -- " + std::to_string(i) + " !!");
            assert(bret);
        }
    }

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    genout ("Stream Client : Client Exit...\n");
    return 0;
}


int testUnixDomainDataGram()
{
    std::shared_ptr<network::TaskDistributor> eventLoop;
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::DataGram> udg;
    bool bret;

    eventLoop = std::make_shared<network::TaskDistributor>(4); // 4个线程

    // 开始事件循环
    bret = eventLoop->start();
    assert(bret);

    repoller = std::make_shared<network::RePoller>();
    udg = std::make_shared<network::DataGram>(base::Socket(AF_UNIX,SOCK_DGRAM,0),repoller);

    ::system(fmt::format("rm {}", domainName).c_str());
    bret = udg->bindAddr(domainName);
    assert(bret);

    std::weak_ptr<network::DataGram> wudg = udg;

    genout("UnixDomain DG : Prepare To Wait Data...\n");

    udg->setOnClose([]()
    {
        m_exit= 1;
        errout("UnixDomain DG : Socket Closed , Prepare Exit...\n");
    });

    udg->setOnWrite([](ssize_t ret, std::string && data)
    {
        if (ret < 200)
        {
            genout("UnixDomain DG : Write Some Data To Remote Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("UnixDomain DG : Write Some Data To Remote Done...Size = [%ld]\t\r.",ret);
        }
        if(ret == -1)
        {
            genout("SUB UnixDomain DG : errstr = %m.\n",errno);
        }
    });


    udg->setOnRead([wudg](ssize_t ret, std::string && data, std::string && saddr_structure [[maybe_unused]] )mutable
    {
        if (ret < 200)
        {
            genout("UnixDomain DG : Read Some Data From Remote[%s], EchoBack ...Size = [%ld]. Content = [%s].\n",domainName.c_str(),ret,data.c_str());
        }
        else
        {
            genout("UnixDomain DG : Read Some Data From Remote[%s], EchoBack ...Size = [%ld]\t\r.",domainName.c_str(),ret);
        }
    });

    // 开始polling
    bret = repoller->start(eventLoop,10);
    assert(bret);

    udg->startListenReadEvent();

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    genout ("UnixDomain DG : UnixDomain DG Exit...\n");
    return 0;
}


int testUnixDomainDataGram_sub()
{
    std::shared_ptr<network::TaskDistributor> eventLoop;
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::DataGram> udg;
    bool bret;

    eventLoop = std::make_shared<network::TaskDistributor>(4); // 4个线程

    // 开始事件循环
    bret = eventLoop->start();
    assert(bret);

    repoller = std::make_shared<network::RePoller>();
    udg = std::make_shared<network::DataGram>(base::Socket(AF_UNIX,SOCK_DGRAM,0),repoller);

    std::weak_ptr<network::DataGram> wudg = udg;

    genout("SUB UnixDomain DG : Prepare To Wait Data...\n");

    udg->setOnClose([]()
    {
        m_exit= 1;
        errout("SUB UnixDomain DG : Socket Closed , Prepare Exit...\n");
    });

    udg->setOnWrite([](ssize_t ret, std::string && data)
    {
        if (ret < 200)
        {
            genout("SUB UnixDomain DG : Write Some Data To Remote Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("SUB UnixDomain DG : Write Some Data To Remote Done...Size = [%ld]\t\r.",ret);
        }
        if(ret == -1)
        {
            genout("SUB UnixDomain DG : errstr = %m.\n",errno);
        }
    });

    udg->setOnRead([wudg](ssize_t ret, std::string && data, std::string && saddr_structure [[maybe_unused]] )mutable
    {
        
        if (ret < 200)
        {
            genout("SUB UnixDomain DG : Read Some Data From Remote[%s], EchoBack ...Size = [%ld]. Content = [%s].\n",domainName.c_str(),ret,data.c_str());
        }
        else
        {
            genout("SUB UnixDomain DG : Read Some Data From Remote[%s], EchoBack ...Size = [%ld]\t\r.",domainName.c_str(),ret);
        }
    });

    // 开始polling
    bret = repoller->start(eventLoop,10);
    assert(bret);

    bret = udg->startListenReadEvent();
    assert(bret);

    /// 
    for (int i =0 ; i< 5 ; i++)
    {
        udg->asend(domainName , "SUB -> hello world-" + std::to_string(i) + " !!");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    genout ("SUB UnixDomain DG : UnixDomain DG Exit...\n");
    return 0;
}

int testServerAndClient()
{
    m_exit = 0;
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   genout("准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        errout("Cannot registering signal handler");
        return -3;
    }

    genout("List : \t1.Unix Domain Socket (Stream) \n\t2.Unix Domain Socket (Datagram) .\nInput :");
    std::string in;
    std::cin >>in;

    int opt = std::stoi(in);

    genout("Config : Socket File Name=%s.\n",domainName.c_str());

    if (opt == 1 )
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            testUnixDomainStreamServer();
        }
        else 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            testUnixDomainStreamClient();
        }
    }
    else if(opt == 2)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            testUnixDomainDataGram();
        }
        else 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            testUnixDomainDataGram_sub();
        }
    }

    return 0;
}



int main(int argc , char ** argv)
{
    testServerAndClient();
    return 0;
}


