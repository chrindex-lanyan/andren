
#include "../include/andren.hh"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

using namespace chrindex::andren::base;

#define errprintf(...) fprintf(stderr, __VA_ARGS__)
#define stdprintf(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit = 0;

std::string serverip = "192.168.88.3";
int32_t serverport = 8317;

bool processSocketEvent(ThreadPool &tpool,
                        Socket &listener, std::any userData)
{
    struct _Private
    {
        Epoll ep;
        EventContain events;
        std::unordered_map<uint64_t, Socket> cliMap;

        _Private(int perEventSize = 100) : events(100) {} // 每次Epoll wait返回的最大事件数量
    };

    std::shared_ptr<_Private> pdata;

    if (m_exit) // 退出
    {
        stdprintf("Server : Epoller Stopped.\n");
        return false;
    }

    if (userData.has_value() && userData.type() == typeid(std::shared_ptr<_Private>))
    {
        pdata = std::any_cast<std::shared_ptr<_Private>>(userData);
    }

    if (!pdata)
    {
        pdata = std::make_shared<_Private>();

        // 添加监听Accept事件
        epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = listener.handle();
        int ret = pdata->ep.control(EpollCTRL::ADD, listener.handle(), event);

        stdprintf("Server : Created Epoll And Add Listener(FD=%d) Event. Result Code = %d.\n", listener.handle(), ret);
    }

    if (int numEvents = pdata->ep.wait(pdata->events, 1000); numEvents < 0)
    {
        errprintf("Server : Epoll Failed.\n");
        return false;
    }
    else if (numEvents == 0)
    {
        stdprintf("Server : Epoll Timeout.\n");
        // return false;
    }
    else if (numEvents > 0)
    {
        stdprintf("Server : Process Events...\n");

        for (int i = 0; i < numEvents; i++)
        {
            auto pevent = pdata->events.reference_ptr(i);
            stdprintf("Server : This Event Code =%d.\n", pevent->events);
            if (pevent->data.fd == listener.handle())
            {
                // accept
                sockaddr_storage _cliaddr;
                sockaddr_in *pcliaddr = reinterpret_cast<sockaddr_in *>(&_cliaddr);
                uint32_t size = sizeof(sockaddr_in);
                Socket cli = listener.accept(reinterpret_cast<sockaddr *>(&_cliaddr), &size /* , SOCK_NONBLOCK | SOCK_CLOEXEC*/);

                stdprintf("Server : Try Accept...FD = %d.\n", cli.handle());
                if (!cli.valid() || size != sizeof(sockaddr_in))
                {
                    errprintf("Server : Client Accept But Server Not Support IPV6...Will Close.\n");
                    cli.close();
                    //::kill(::getpid(),SIGINT); // 给自己发信号结束
                    continue;
                }
                int32_t clifd = cli.handle();
                uint64_t cliaddrkey = ((uint64_t(0) + pcliaddr->sin_addr.s_addr) << 32) + pcliaddr->sin_port;
                pdata->cliMap[cliaddrkey] = std::move(cli);
                EndPointIPV4 cliep{pcliaddr};
                stdprintf("Server : Server Accept [%s:%d].\n", cliep.ip().c_str(), cliep.port());

                // 注册读事件
                epoll_event cliev;
                cliev.data.u64 = cliaddrkey;
                cliev.events = EPOLLIN;
                pdata->ep.control(EpollCTRL::ADD, clifd, cliev);
            }
            else if (pevent->events & EPOLLIN)
            {
                // client
                stdprintf("Server : Client Event...\n");
                if (auto iter = pdata->cliMap.find(pevent->data.u64); iter != pdata->cliMap.end())
                {
                    Socket &cli = iter->second;
                    std::string buffer, tmp(128, 0xff);
                    bool readok = false;
                    stdprintf("Server : Client Read Event...\n");
                    while (1)
                    {
                        ssize_t ret = cli.recv(&tmp[0], tmp.size(), MSG_DONTWAIT);
                        
                        if (int errcode = errno; ret < 0 && (errcode == EAGAIN || errcode == EWOULDBLOCK))
                        {
                            // read done.
                            readok = true;
                            stdprintf("Server : Client's Data Read Done...\n");
                            break;
                        }
                        else if (ret == 0 || ret <0)
                        {
                            // disconnect
                            pdata->ep.control(EpollCTRL::DEL, cli.handle(), {});
                            pdata->cliMap.erase(iter);
                            stdprintf("Server : Client disconnect...\n");
                            break;
                        }
                        else if (ret > 0)
                        {
                            buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + ret);
                            stdprintf("Server : Client Append Data.\n");
                        }
                    }
                    if (readok)
                    {
                        stdprintf("Server : Read From Client [%s]...EchoBack.\n", buffer.c_str());
                        cli.send(buffer.c_str(),buffer.size(),0);
                    }
                }
            } // end else if (pevent->events & EPOLLIN)
        }     // end for
    }         // end else if (numEvents > 0)

    tpool.exec([&tpool, &listener, _pdata = std::move(pdata)]()
               { processSocketEvent(tpool, listener, std::move(_pdata)); },
               0);
    return true;
}

int test_server()
{
    Socket listener(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    EndPointIPV4 saddr(serverip, serverport);

    if (!listener.valid())
    {
        errprintf("Server : cannot create listen socket.\n");
        return -1;
    }
    int optval = 1;
    listener.setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (listener.bind(saddr.toAddr(), saddr.addrSize()))
    {
        errprintf("Server : cannot bind sockaddr in.\n");
        return -2;
    }
    if (listener.listen(100))
    {
        errprintf("Server : cannot listen tcp socket.\n");
        return -3;
    }
    stdprintf("Server : listening [%s:%d] with Socket fd %d.\n", saddr.ip().c_str(), saddr.port(), listener.handle());

    // 设置CTRL+C信号回调
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   stdprintf("Server : 准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        errprintf("Server : cannot registering signal handler");
        return -3;
    }

    ThreadPool tpool(4); // four threads

    // 开始执行
    tpool.exec([&tpool, &listener]()
               { processSocketEvent(tpool, listener, {}); },
               0);

    /// 管程等信号就得了
    while (!(int)m_exit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    stdprintf("Server : Main thread prepare exit...Exit Flags = %d.\n", (int)m_exit);
    Thread::pthread_self_exit();

    return 0;
}

int test_client()
{
    Socket csock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    EndPointIPV4 ep(serverip, serverport);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (csock.connect(ep.toAddr(), ep.addrSize()))
    {
        errprintf("Client : Connect server failed.Errcode = %d::%m\n", errno,errno);
        return -1;
    }

    Epoll epoll;
    epoll_event event;
    EventContain ecc(10);
    std::string data;

    event.data.fd = csock.handle();
    event.events = EPOLLIN;

    int ret = epoll.control(EpollCTRL::ADD, csock.handle(), event);
    stdprintf("Client : Add Read Event...ret = %d.\n",ret);

    for (int i = 0; m_exit !=1 ; i++)
    {
        if (i < 100)
        {
            data = "hand-shake-" + std::to_string(i)+"\t";
        }
        else
        {
            data.resize(0);
        }

        if (!data.empty())
        {
            event.data.fd = csock.handle();
            event.events = EPOLLIN | EPOLLOUT;
            ret = epoll.control(EpollCTRL::MOD, csock.handle(), event);
            stdprintf("Client : Add Write Event...ret = %d.\n",ret);
        }
        int num = epoll.wait(ecc, 3000);
        if (num == 0)
        {
            stdprintf("Client : epoll wait timeout.\n");
            continue;
        }
        else if (num < 0)
        {
            errprintf("Client : epoll wait failed.\n");
            break;
        }
        for (int k = 0; k < num; k++)
        {
            auto pevent = ecc.reference_ptr(k);
            if (pevent->data.fd != csock.handle())
            {
                continue;
            }
            if (pevent->events & EPOLLIN) // readable
            {
                std::string rddata , tmp(128,0);
                bool readok = false;
                stdprintf("Client : Read Event...\n");
                while (1)
                {
                    ssize_t ret = csock.recv(&tmp[0], tmp.size(), MSG_DONTWAIT);
                    
                    if (int errcode = errno; ret < 0 && (errcode == EAGAIN || errcode == EWOULDBLOCK))
                    {
                        // read done.
                        readok = true;
                        stdprintf("Client : Data Read Done...\n");
                        break;
                    }
                    else if (ret == 0 || ret < 0)
                    {
                        // disconnect
                        stdprintf("Client : disconnect...\n");
                        break;
                    }
                    else if (ret > 0)
                    {
                        rddata.insert(rddata.end(), tmp.begin(), tmp.begin() + ret);
                        stdprintf("Client : Append Data.\n");
                    }
                }
                if (readok)
                {
                    stdprintf("Client : Read From Server [%s].\n", rddata.c_str());
                }
            }
            if (pevent->events & EPOLLOUT) // writable
            {
                csock.send(data.c_str(),data.size(),0);
                event.data.fd = csock.handle();
                event.events = EPOLLIN;
                epoll.control(EpollCTRL::MOD, csock.handle(), event);
            }
        }
    }

    return 0;
}

int test_socket()
{
    pid_t pid = fork();

    if (pid != 0)
    { // server
        test_server();
    }
    else
    { // client
        test_client();
    }
    return 0;
}

int main(int argc, char **argv)
{
    test_socket();
    return 0;
}
