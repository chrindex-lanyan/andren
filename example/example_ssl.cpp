
#include "../include/andren.hh"

#include <chrono>
#include <thread>
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

std::string prevPath = "./test_key/";

std::string serverPrimaryKeyPath = prevPath + "server_private.key";
std::string serverCertificatePath = prevPath + "server_certificate.crt";

std::string clientPrimaryKeyPath = prevPath + "client_private.key";
std::string clientCertificatePath = prevPath + "client_certificate.crt";

bool processSocketEvent(ThreadPoolPortable &tpool,
                        Socket &listener, std::any userData)
{
    struct _Private
    {
        struct _SSL_Socket
        {
            Socket sock;
            aSSLSocketIO sio;

            _SSL_Socket() {}
            _SSL_Socket(Socket &&sock, aSSLSocketIO &&sio)
            {
                this->sock = std::move(sock);
                this->sio = std::move(sio);
            }
            _SSL_Socket(_SSL_Socket &&_)
            {
                sock = std::move(_.sock);
                sio = std::move(_.sio);
            }
            void operator=(_SSL_Socket &&_)
            {
                sock = std::move(_.sock);
                sio = std::move(_.sio);
            }
        };

        EventContain events;
        Epoll ep;
        std::unordered_map<uint64_t, _SSL_Socket> cliMap;
        aSSLContextCreator creator;
        aSSLContext sslCtx;

        _Private(int perEventSize = 100) : events(100) {} // 每次Epoll wait返回的最大事件数量
    };
    std::shared_ptr<_Private> pdata;

    if (m_exit) // 退出
    {
        stdprintf("SSL Server : Epoller Stopped.\n");
        return false;
    }

    if (userData.has_value() && userData.type() == typeid(std::shared_ptr<_Private>))
    {
        pdata = std::any_cast<std::shared_ptr<_Private>>(userData);
    }

    if (!pdata)
    {
        pdata = std::make_shared<_Private>();

        // 完成SSL Context的初始化
        aSSLContextCreator &creator = pdata->creator;

        creator.setPrimaryKeyFilePath(serverPrimaryKeyPath);
        creator.setCertificateFileFilePath(serverCertificatePath);
        creator.setOptionsFlags(
            SSL_OP_ALL                                      // 启用全部选项
            | SSL_OP_NO_SSLv2                               // 禁用SSLv2，因为它不安全
            | SSL_OP_NO_SSLv3                               // 禁用SSLv3，因为它不安全
            | SSL_OP_NO_COMPRESSION                         // 禁用压缩，因为它不安全
            | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION // 在重协商时不恢复会话，增加安全性。
        ).setSupportedProtoForServer(
            {
                {
                    "directly", []() -> bool
                    {
                        stdprintf("SSL Server : Client Support Protocol `directly`.\n");
                        return true;
                    }
                },
                {
                    "stream", []() -> bool
                    {
                        stdprintf("SSL Server : Client Support Protocol `stream`.\n");
                        return true;
                    }
                }
            });

        if (auto pctx = creator.startCreate(1); pctx.key()==0)
        {
            pdata->sslCtx = pctx.value();
            stdprintf("SSL Server : Create OpenSSL Context Done.\n");
        }
        else
        {
            errprintf("SSL Server : Create OpenSSL Context Failed. ErrCode=%d.\n",pctx.key());
            return false;
        }

        // 添加监听Accept事件
        epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = listener.handle();
        int ret = pdata->ep.control(EpollCTRL::ADD, listener.handle(), event);

        stdprintf("SSL Server : Created Epoll And Add Listener(FD=%d) Event. Result Code = %d.\n", listener.handle(), ret);
    }

    if (int numEvents = pdata->ep.wait(pdata->events, 1000); numEvents < 0)
    {
        errprintf("SSL Server : Epoll Failed.\n");
        return false;
    }
    else if (numEvents == 0)
    {
        stdprintf("SSL Server : Epoll Timeout.\n");
        // return false;
    }
    else if (numEvents > 0)
    {
        stdprintf("SSL Server : Process Events...\n");

        for (int i = 0; i < numEvents; i++)
        {
            auto pevent = pdata->events.reference_ptr(i);
            stdprintf("SSL Server : This Event Code =%d.\n", pevent->events);
            if (pevent->data.fd == listener.handle())
            {
                // accept
                sockaddr_storage _cliaddr;
                sockaddr_in *pcliaddr = reinterpret_cast<sockaddr_in *>(&_cliaddr);
                uint32_t size = sizeof(sockaddr_in);
                Socket cli = listener.accept(reinterpret_cast<sockaddr *>(&_cliaddr), &size /* , SOCK_NONBLOCK | SOCK_CLOEXEC*/);
                EndPointIPV4 cliep{pcliaddr};

                stdprintf("SSL Server : Try Accept...FD = %d.\n", cli.handle());
                if (!cli.valid() || size != sizeof(sockaddr_in))
                {
                    errprintf("SSL Server : Client Accept But Server Not Support IPV6...Will Close.\n");
                    cli.close();
                    //::kill(::getpid(),SIGINT); // 给自己发信号结束
                    continue;
                }
                stdprintf("SSL Server : Server Accept [%s:%d].\n", cliep.ip().c_str(), cliep.port());

                int32_t clifd = cli.handle();
                uint64_t cliaddrkey = ((uint64_t(0) + pcliaddr->sin_addr.s_addr) << 32) + pcliaddr->sin_port;

                /// 处理SSL相关
                aSSLSocketIO asslio;

                asslio.upgradeFromSSL(pdata->sslCtx.handle());
                asslio.bindSocketFD(cli);
                asslio.setEndType(1);// 服务端
                if (int ret = asslio.handShake(); ret == 1)
                {
                    // OK
                    stdprintf("SSL Server : SSL Hand-Shake Done...\n");
                    pdata->cliMap[cliaddrkey] = {std::move(cli), std::move(asslio)};
                }
                else if (ret == 0 || ret == -1)
                {
                    // failed.
                    errprintf("SSL Server : SSL Hand-Shake Failed....ret=%d.SSL_err=%d.SysErr=%d.SocketFD=%d.\n",
                        ret,
                        asslio.reference().getSSLError(ret),
                        errno,
                        clifd);
                    continue;
                }

                // 注册读事件
                epoll_event cliev;
                cliev.data.u64 = cliaddrkey;
                cliev.events = EPOLLIN;
                pdata->ep.control(EpollCTRL::ADD, clifd, cliev);
            }
            else if (pevent->events & EPOLLIN)
            {
                // client
                stdprintf("SSL Server : Client Event...\n");
                if (auto iter = pdata->cliMap.find(pevent->data.u64); iter != pdata->cliMap.end())
                {
                    aSSLSocketIO &asslio = iter->second.sio;
                    // Socket &cli = iter->second.sock;
                    stdprintf("SSL Server : Client Read Event...\n");

                    KVPair<ssize_t , std::string> result = asslio.read();
                    
                    if (result.key() ==0 )
                    {
                        // disconnect
                        pdata->ep.control(EpollCTRL::DEL, pevent->data.fd, {});
                        pdata->cliMap.erase(iter);
                        stdprintf("SSL Server : Client Disconnect...\n");
                    }
                    else if(result.key() < 0)
                    {
                        stdprintf("SSL Server : Retry Read Client's Data...\n");
                    }
                    else 
                    {
                        stdprintf("SSL Server : Read From Client [%s]...EchoBack,Size=%ld\n", 
                            result.value().c_str(),
                            result.value().size()
                            );
                        asslio.write(std::move(result.value()));
                    }
                }
            } // end else if (pevent->events & EPOLLIN)
        }     // end for
    }

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
        errprintf("SSL Server : cannot create listen socket.\n");
        return -1;
    }
    int optval = 1;
    listener.setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (listener.bind(saddr.toAddr(), saddr.addrSize()))
    {
        errprintf("SSL Server : cannot bind sockaddr in.\n");
        return -2;
    }
    if (listener.listen(100))
    {
        errprintf("SSL Server : cannot listen tcp socket.\n");
        return -3;
    }
    stdprintf("SSL Server : Listening [%s:%d] with Socket fd %d.\n", saddr.ip().c_str(), saddr.port(), listener.handle());

    ThreadPoolPortable tpool(4); // four threads

    // 开始执行
    tpool.exec([&tpool, &listener]()
               { processSocketEvent(tpool, listener, {}); },
               0);

    /// 管程等信号就得了
    while (!(int)m_exit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    stdprintf("SSL Server : Main thread prepare exit...Exit Flags = %d.\n", (int)m_exit);
    return 0;
}

int test_client()
{
    Socket csock(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    EndPointIPV4 ep(serverip, serverport);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (csock.connect(ep.toAddr(), ep.addrSize()))
    {
        errprintf("SSL Client : Connect server failed.Errcode = %d::%m\n", errno, errno);
        return -1;
    }

    /// 配置SSL
    aSSLContextCreator creator;

    creator.setPrimaryKeyFilePath(clientPrimaryKeyPath)
        .setCertificateFileFilePath(clientCertificatePath)
        .setOptionsFlags(SSL_OP_ALL                                      // 启用全部选项
                         | SSL_OP_NO_SSLv2                               // 禁用SSLv2，因为它不安全
                         | SSL_OP_NO_SSLv3                               // 禁用SSLv3，因为它不安全
                         | SSL_OP_NO_COMPRESSION                         // 禁用压缩，因为它不安全
                         | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION // 在重协商时不恢复会话，增加安全性。
                         )
        .setSupportedProtoForClient(
            {
                {
                    "directly",
                    {} // 客户端无需设置协议回调函数
               },
               {
                    "stream", 
                    {}
               }
            });

    auto result = creator.startCreate(2);
    if (result.key()!=0)
    {
        errprintf("SSL Client : Cannot Create SSL Context.ErrCode=%d.\n",result.key());
        return -2;
    }

    aSSL assl = result.value();
    aSSLSocketIO asslio;
    std::string protocol;

    asslio.upgradeFromSSL(std::move(assl));
    asslio.bindSocketFD(csock);
    asslio.setEndType(2);// 客户端

    if (int ret = asslio.initiateHandShake(); ret !=1 )
    {
        errprintf("SSL Client : Cannot Initiate SSL Hand-Shake. Return Code =%d. SSL_ERRORS=%d.SysErrCode=%d.\n",
            ret,
            asslio.reference().getSSLError(ret),
            errno
            );
        return -3;
    }
    protocol = asslio.reference().selectedProtocolForClient(); // 取得已经协商好的应用层协议 
    if(protocol.empty()){protocol = "null";}
    stdprintf("SSL Client : Using Protocol `%s`. Size=%lu.\n", protocol.c_str(), protocol.size());

    /// 配置Epoll
    epoll_event event;
    EventContain ecc(10);
    Epoll epoll;
    std::string data;

    event.data.fd = csock.handle();
    event.events = EPOLLIN;

    int ret = epoll.control(EpollCTRL::ADD, csock.handle(), event);
    stdprintf("SSL Client : Add Read Event...ret = %d.\n", ret);

    for (int i = 0; m_exit != 1; i++)
    {
        if (i < 100)
        {
            data = "i am client -" + std::to_string(i) ;
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
            stdprintf("SSL Client : Add Write Event...ret = %d.\n", ret);
        }
        int num = epoll.wait(ecc, 3000);
        if (num == 0)
        {
            stdprintf("SSL Client : Epoll wait timeout.\n");
            continue;
        }
        else if (num < 0)
        {
            errprintf("SSL Client : Epoll wait failed.\n");
            break;
        }
        for (int k = 0; k < num && m_exit != 1; k++)
        {
            auto pevent = ecc.reference_ptr(k);
            if (pevent->data.fd != csock.handle())
            {
                continue;
            }
            if (pevent->events & EPOLLIN) // readable
            {
                stdprintf("SSL Client : Read Event...\n");

                KVPair<ssize_t, std::string> result = asslio.read();

                if (result.key() == 0)
                {
                    // disconnect
                    stdprintf("SSL Client : Disconnect...\n");
                    return 0;
                }
                else if(result.key() <=0 )
                {
                    // re-try
                    stdprintf("SSL Client : Retry received data from SSL_read...\n");
                }else 
                {
                    stdprintf("SSL Client : Read From Server [%s].\n", result.value().c_str());
                }
            }
            if (pevent->events & EPOLLOUT) // writable
            {
                asslio.write(data);
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

    if (signal(SIGINT,
               [](int sig) -> void
               {
                   fprintf(stdout,"准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        fprintf(stdout,"Cannot registering signal handler");
        return -3;
    }

    if (pid != 0)
    { 
        // server
        fprintf(stdout,"Parent PID %d.\n",::getpid());
        test_server();
        // while(m_exit != 1)
        // {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // }
    }
    else
    { 
        // client
        fprintf(stdout,"Child PID %d.\n",::getpid());
        test_client();
    }
    return 0;
}

int main(int argc, char **argv)
{
    test_socket();
    return 0;
}
