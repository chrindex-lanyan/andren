
#include "../include/andren.hh"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

using namespace chrindex::andren::base;

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define sprintf(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit = 0;

bool processSocketEvent(ThreadPool &tpool,
                        Socket &listener, void *userData)
{
    struct _Private
    {
        Epoll ep;
        EventContain events; // 每次Epoll wait返回的最大事件数量
        epoll_event event;
        std::unordered_map<uint64_t, Socket> cliMap;

        _Private(int perEventSize = 100): events(100)
        {
            memset(&event, 0, sizeof(event));
        }
    };

    _Private *pdata = reinterpret_cast<_Private *>(userData);

    if (m_exit)
    {
        // 退出
        eprintf("Epoller Stopped.\n");
        return false;
    }

    if (pdata == 0)
    {
        pdata = new _Private;

        // 添加监听Accept事件
        pdata->event.events = EPOLLIN;
        pdata->event.data.fd = listener.handle();
        pdata->ep.control(EpollCTRL::ADD, listener.handle(), pdata->event );
    }

    if(int numEvents; numEvents =  pdata->ep.wait(pdata->events , 5))
    {
        if (numEvents < 0)
        {
            eprintf("Epoll Failed.\n");
            return false;
        }
        for (int i = 0; i < numEvents ; i++)
        {
            auto pevent = pdata->events.reference_ptr(i);
            if (pevent->data.fd == listener.handle()) 
            {
                // accept
                
            }else 
            {
                // client

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
    EndPointIPV4 saddr("127.0.0.1", 8317);

    if (listener.valid())
    {
        eprintf("cannot create listen socket.\n");
        return -1;
    }
    if (listener.bind(saddr.toAddr(), saddr.addrSize()))
    {
        eprintf("cannot bind sockaddr in.\n");
        return -2;
    }

    ThreadPool tpool(4);

    if (signal(SIGINT, [](int sig) -> void
               {
        sprintf("Server准备退出....\n");
        m_exit = 1; }) == SIG_ERR)
    {
        eprintf("Error registering signal handler");
        return -3;
    }

    // 开始执行
    tpool.exec([&tpool, &listener]()
               { processSocketEvent(tpool, listener, 0); },
               0);

    /// 管程等信号就得了
    while (m_exit != 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    Thread::pthread_self_exit();

    return 0;
}

int test_client()
{

    return 0;
}

int test_ssl()
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
    test_ssl();
    return 0;
}
