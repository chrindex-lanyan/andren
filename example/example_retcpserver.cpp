
#include "../src/network/andren_network.hh"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <iostream>


using namespace chrindex::andren;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit;

std::string serverip = "192.168.88.3";
int32_t serverport = 8317;


int testTcpServer()
{
    std::shared_ptr<network::EventLoop> eventLoop;
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::Acceptor> acceptor;
    base::Socket ssock(AF_INET, SOCK_STREAM,0);
    base::EndPointIPV4 epv4(serverip,serverport);
    int ret ;
    bool bret;

    eventLoop = std::make_shared<network::EventLoop>(4); // 4个线程

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
            m_exit= true;
            errout("TCP Server : Accept Faild! Socket Error.\n");
            return ;
        }
        int fd = link->reference_handle()->handle();
        std::weak_ptr<network::SockStream> wlink = link;
        std::weak_ptr<network::RePoller> wrepoller = link->reference_repoller();
        std::shared_ptr<network::RePoller> repoller = wrepoller.lock();

        link->setOnClose([wrepoller , fd]() 
        {
            //m_exit= true;
            //errout("TCP Server : Client Disconnected , Server Exit...\n");
            errout("TCP Server : Client Disconnected .\n");
            std::shared_ptr<network::RePoller> repoller = wrepoller.lock();
            if (!repoller)
            {
                return ;
            }

            /// 将link从对象池里去除
            repoller->findObject(fd, true , [ ](bool ret, std::any * obj)
            {
                genout("TCP Server : This Client Is TakeOut From This ObjectsMap.\n");
            });
            return ;
        });

        link->setOnWrite([ ](ssize_t ret, std::string && data) 
        {
            genout("TCP Server : Write Some Data To Client Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
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
            if(link)
            {
                link->asend(std::move(data)); // 返回true时，该函数会保存一份link强引用，直到send完成。
            }
        });

        /// 监听读取事件
        bool bret = link->startListenReadEvent();
        assert(bret);

        /// 强引用保存且仅保存到对象池
        /// 反正用到的时候可以查
        bret = repoller->saveObject(fd, true, link, [](bool ret, std::any * obj)
        {
            genout("TCP Server : This Client Is Saved Into The ObjectsMap.\n");
        });
        assert(bret);
    });

    // 开始polling
    bret = repoller->start(eventLoop,2);
    assert(bret);

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
    genout ("TCP Server : Server Exit...\n");
    return 0;
}

int testTcpClient()
{
    std::shared_ptr<network::EventLoop> eventLoop;
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::SockStream> link;
    base::Socket csock(AF_INET, SOCK_STREAM,0);
    bool bret;

    eventLoop = std::make_shared<network::EventLoop>(4); // 4个线程

    // 开始事件循环
    bret = eventLoop->start();
    assert(bret);

    repoller = std::make_shared<network::RePoller>();
    link = std::make_shared<network::SockStream>(std::move(csock),repoller);

    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    std::weak_ptr<network::SockStream> wlink = link;

    genout("TCP Client : Prepare To  Connect Server...\n");

    link->setOnClose([]()
    {
        m_exit= true;
        errout("TCP Client : Disconnected , Prepare Exit...\n");
    });

    link->setOnWrite([](ssize_t ret, std::string && data)
    {
        if (ret < 200)
        {
            genout("TCP Client : Write Some Data To Server Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("TCP Client : Write Some Data To Server Done...Size = [%ld]\t\r.",ret);
        }
    });

    link->setOnRead([wlink](ssize_t ret, std::string && data)mutable
    {
        if (ret < 200)
        {
            genout("TCP Client : Read Some Data From Server, EchoBack ...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("TCP Client : Read Some Data From Server, EchoBack ...Size = [%ld]\t\r.",ret);
        }
        
        auto link = wlink.lock();
        if (link)
        {
            link->asend(std::move(data)); // 返回true时，该函数会保存一份link强引用，直到send完成。
            link.reset();
        }
    });

    // windows host = "192.168.88.2";

    // 开始polling
    bret = repoller->start(eventLoop,5);
    assert(bret);

    /// 注意，要先polling才能asyncConnect
    bret = link->asyncConnect( serverip, serverport, [link, &promise](bool ok)
    {
        genout("TCP Client : OnConnected = %s.\n",(ok?"True":"False"));
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
        errout("TCP Client : Connect Failed...Exit...\n");
        m_exit=1;
        goto end_clean;
    }

    for (int i =0; i< 10 ; i++)
    {
        bret = link->asend("hello world-" + std::to_string(i) + " !!");
        assert(bret);
    }

end_clean:

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    genout ("TCP Client : Client Exit...\n");
    return 0;
}


int testTCPServerAndClient()
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

    genout("List : \t1.TCP SERVER \n\t2.TCP CLIENT .\nInput :");
    std::string in;
    std::cin >>in;

    int opt = std::stoi(in);

    genout("Please Key in IP.Enter `default` Content To Using Default-Value.Default = %s.\nInput :",serverip.c_str());
    std::cin >> in;
    if (in != "default")
    {
        serverip = in;
    }
        genout("Please Key in Port.Enter `default` Content To Using Default-Value.Default = %d.\nInput :",serverport);
    std::cin >> in;
    if (in != "default" )
    {
        serverport = std::stoi(in);
    }

    genout("Config : ip=%s,port=%d.\n",serverip.c_str(),serverport);

    if (opt == 1 )
    {
        testTcpServer();
        genout("TCP Server : Exit ...\n");
    }
    else if(opt == 2)
    {
        testTcpClient();
        genout("TCP Client : Exit ...\n");
    }

    return 0;
}



int main(int argc , char ** argv)
{
    testTCPServerAndClient();
    return 0;
}



