﻿
#include "../include/andren.hh"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

using namespace chrindex::andren::base;

#define errprintf(...) fprintf(stderr, __VA_ARGS__)
#define stdprintf(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit = 0;

bool processSocketEvent(ThreadPool &tpool,
                        Socket &listener, void *userData)
{
    struct _Private
    {
        Epoll ep;
        EventContain events; // 每次Epoll wait返回的最大事件数量
        std::unordered_map<uint64_t, Socket> cliMap;

        _Private(int perEventSize = 100): events(100)
        {
            //
        }
    };

    _Private *pdata = reinterpret_cast<_Private *>(userData);

    if (m_exit)
    {
        // 退出
        errprintf("Epoller Stopped.\n");
        return false;
    }

    if (pdata == 0)
    {
        pdata = new _Private;

        // 添加监听Accept事件
        epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = listener.handle();
        int ret = pdata->ep.control(EpollCTRL::ADD, listener.handle(), event);

        stdprintf("Created Epoll And Add Listener(FD=%d) Event. Result Code = %d.\n",listener.handle(),ret);
    }

    if(int numEvents = pdata->ep.wait(pdata->events , 1000); numEvents < 0)
    {
        errprintf("Epoll Failed.\n");
        return false;
    }else if(numEvents == 0) 
    {
        errprintf("Epoll Timeout.\n");
        //return false;
    }else if (numEvents > 0)
    {
        stdprintf("Process Events...\n");
        
        for (int i = 0; i < numEvents ; i++)
        {
            auto pevent = pdata->events.reference_ptr(i);
            stdprintf("This Event Code =%d.\n",pevent->events);
            if (pevent->data.fd == listener.handle()) 
            {
                // accept
                sockaddr_storage _cliaddr;
                sockaddr_in * pcliaddr = reinterpret_cast<sockaddr_in*>(&_cliaddr);
                uint32_t size = sizeof(sockaddr_in);
                Socket cli  = listener.accept(reinterpret_cast<sockaddr*>(&_cliaddr),&size /* , SOCK_NONBLOCK | SOCK_CLOEXEC*/);

                stdprintf("Try Accept...FD = %d.\n",cli.handle());
                if (!cli.valid() || size != sizeof(sockaddr_in))
                {
                    errprintf("Client Accept But Server Not Support IPV6...Will Close.\n");
                    cli.close();
                    //::kill(::getpid(),SIGINT); // 给自己发信号结束
                    continue;
                }
                int32_t clifd = cli.handle();
                uint64_t cliaddrkey 
                    = ((uint64_t(0) + pcliaddr->sin_addr.s_addr) << 32) + pcliaddr->sin_port ;
                pdata->cliMap[cliaddrkey] = std::move(cli);
                EndPointIPV4 cliep{pcliaddr};
                stdprintf("Server Accept [%s:%d].\n", cliep.ip().c_str(), cliep.port() );

                // 注册读事件
                epoll_event cliev ;
                cliev.data.u64 = cliaddrkey;
                cliev.events = EPOLLIN;
                pdata->ep.control(EpollCTRL::ADD, clifd , cliev);
            }else if (pevent->events & EPOLLIN)
            {
                // client
                stdprintf("Client Event...\n");
                if (auto iter = pdata->cliMap.find(pevent->data.u64);iter != pdata->cliMap.end())
                {
                    Socket & cli = iter->second;
                    std::string buffer, tmp(128,0xff);
                    bool readok= false;
                    stdprintf("Client Read Event...\n");
                    while(1)
                    {
                        ssize_t ret = cli.recv(&tmp[0],tmp.size(),MSG_DONTWAIT);
                        if (ret == 0)
                        {
                            // disconnect
                            pdata->ep.control(EpollCTRL::DEL,cli.handle(),{});
                            pdata->cliMap.erase(iter);
                            stdprintf("Client disconnect...\n");
                            break;
                        }
                        else if(int errcode = errno ; ret <0 && (errcode == EAGAIN || errcode== EWOULDBLOCK ))
                        {
                            // read done.
                            readok = true;
                            stdprintf("Client's Data Read Done...\n");
                            break;
                        }
                        else if (ret > 0)
                        {
                            buffer.insert(buffer.end(),tmp.begin(),tmp.begin()+ret);
                            stdprintf("Client Append Data.\n");
                        }
                    }
                    if (readok)
                    {
                        stdprintf("Read From Client:[%s].\n",buffer.c_str());
                    }
                }
                
            }
        }
    }

    tpool.exec([&tpool,  &listener, pdata]()
               { processSocketEvent(tpool,  listener, pdata); },
               0);

    return true;
}

int test_server()
{
    Socket listener(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    EndPointIPV4 saddr("192.168.88.3", 8317);

    if (!listener.valid())
    {
        errprintf("cannot create listen socket.\n");
        return -1;
    }
    int optval = 1;
    listener.setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (listener.bind(saddr.toAddr(), saddr.addrSize()))
    {
        errprintf("cannot bind sockaddr in.\n");
        return -2;
    }
    if(listener.listen(100))
    {
        errprintf("cannot listen tcp socket.\n");
        return -3;
    }
    stdprintf("listening [%s:%d] with Socket fd %d.\n",saddr.ip().c_str(),saddr.port(),listener.handle());

    ThreadPool tpool(4);

    if (signal(SIGINT, 
            [](int sig) -> void
            {
                stdprintf("Server准备退出....\n");
                m_exit = 1; 
            }) == SIG_ERR)
    {
        errprintf("Error registering signal handler");
        return -3;
    }

    // 开始执行
    tpool.exec([&tpool, &listener]()
               { processSocketEvent(tpool, listener, 0); },
               0);

    /// 管程等信号就得了
    while (!(int)m_exit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    stdprintf("Main thread prepare exit...Exit Flags = %d.\n",(int)m_exit);
    Thread::pthread_self_exit();

    return 0;
}

int test_client()
{

    return 0;
}

int test_socket()
{
    pid_t pid = fork();

    if (pid != 0)
    {
        // server
        test_server();
    }
    else
    {
        // client
        test_client();
    }

    return 0;
}

int main(int argc, char **argv)
{
    test_socket();
    return 0;
}
