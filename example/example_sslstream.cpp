
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

std::string prevPath = "./test_key/";

std::string serverPrimaryKeyPath = prevPath + "server_private.key";
std::string serverCertificatePath = prevPath + "server_certificate.crt";

std::string clientPrimaryKeyPath = prevPath + "client_private.key";
std::string clientCertificatePath = prevPath + "client_certificate.crt";

base::aSSL createSSLFromCtx(base::aSSLContext & ctx)
{
    return ctx.handle();
}

bool config_server_ssl(base::aSSLContextCreator &creator, base::aSSLContext & ctx)
{
    creator.setPrimaryKeyFilePath(serverPrimaryKeyPath);
    creator.setCertificateFileFilePath(serverCertificatePath);
    creator.setOptionsFlags(
        SSL_OP_ALL                                // 启用全部选项
        | SSL_OP_NO_SSLv2                               // 禁用SSLv2，因为它不安全
        | SSL_OP_NO_SSLv3                               // 禁用SSLv3，因为它不安全
        | SSL_OP_NO_COMPRESSION                         // 禁用压缩，因为它不安全
        | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION // 在重协商时不恢复会话，增加安全性。
    ).setSupportedProtoForServer(
        {
            {
                "h2", []() -> bool
                {
                    genout("SSL Server : Client Support Protocol `h2`.\n");
                    return true;
                }
            },
            {
                "h2c", []() -> bool
                {
                    genout("SSL Server : Client Support Protocol `h2c`.\n");
                    return true;
                }
            }
        });
    if (auto pctx = creator.startCreate(1); pctx.key()==0)
    {
        ctx = pctx.value();
        genout("SSL Server : Create OpenSSL Context Done.\n");
    }
    else
    {
        errout("SSL Server : Create OpenSSL Context Failed. ErrCode=%d.\n",pctx.key());
        return false;
    }
    return true;
}

int testTcpServer()
{
    base::aSSLContextCreator creator;
    base::aSSLContext sslctx;
    std::shared_ptr<network::EventLoop> eventLoop = std::make_shared<network::EventLoop>(4); // 4个线程
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::Acceptor> acceptor;
    base::Socket ssock(AF_INET, SOCK_STREAM,0);
    base::EndPointIPV4 epv4(serverip,serverport);
    int ret =0;
    bool bret=false;

    // 配置SSL
    bret=config_server_ssl(creator, sslctx);
    assert(bret);

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

    genout("SSL Server : Start Listen And Prepare Accept...\n");

    acceptor->setOnAccept([ &sslctx ](std::shared_ptr<network::SockStream> raw_link)
    {
        if(!raw_link) 
        {
            m_exit= 1;
            errout("SSL Server : Accept Faild! Socket Error.\n");
            return ;
        }
        int fd = raw_link->reference_handle()->handle();

        std::shared_ptr<network::SSLStream> ssllink = std::make_shared<network::SSLStream>(std::move(*raw_link));

        std::weak_ptr<network::SSLStream> wssllink = ssllink;
        std::weak_ptr<network::RePoller> wrepoller = ssllink->reference_repoller();
        std::shared_ptr<network::RePoller> repoller = wrepoller.lock();

        /// Configurate OpenSSL 
        base::aSSL assl = createSSLFromCtx(sslctx); 
        bool bret = ssllink->usingSSL(std::move(assl), 1);
        assert(bret);

        int ret = ssllink->reference_sslio()->handShake();
        assert(ret ==1);

        ssllink->setOnClose([wrepoller , fd]() 
        {
            errout("SSL Server : Client Disconnected .\n");
            std::shared_ptr<network::RePoller> repoller = wrepoller.lock();
            if (!repoller)
            {
                return ;
            }

            /// 将link从对象池里去除
            repoller->findObject(fd, true , [ ](bool ret, std::any * obj)
            {
                genout("SSL Server : This Client Is TakeOut From The ObjectsMap.\n");
            });
            return ;
        });

        ssllink->setOnWrite([ ](ssize_t ret, std::string && data) 
        {
            genout("SSL Server : Write Some Data To Client Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        });

        // weak防止循环引用
        ssllink->setOnRead([wssllink](ssize_t ret, std::string && data) mutable
        {
            if (ret < 200) // 太大则不打印内容
            {
                genout("SSL Server : Read Some Data From Client ...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
            }else 
            {
                genout("SSL Server : Read Some Data From Client ...Size = [%ld].\t\r",ret);
            }
            auto ssllink =wssllink.lock();
            if(ssllink)
            {
                ssllink->asend(std::move(data)); // 返回true时，该函数会保存一份link强引用，直到send完成。
            }
        });

        /// 监听读取事件
        bret = ssllink->startListenReadEvent();
        assert(bret);

        /// 强引用保存且仅保存到对象池
        /// 反正用到的时候可以查
        bret = repoller->saveObject(fd, true, ssllink, [](bool ret, std::any * obj)
        {
            genout("SSL Server : This Client Is Saved Into The ObjectsMap.\n");
        });
        assert(bret);
    });

    // 开始polling
    bret = repoller->start(eventLoop,5);
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
    genout ("SSL Server : Server Exit...\n");
    return 0;
}

bool config_client_ssl(base::aSSLContextCreator &creator , base::aSSLContext & ctx)
{
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
                    "h2",
                    {} // 客户端无需设置协议回调函数
               },
               {
                    "h2c", 
                    {}
               }
            });

    auto result = creator.startCreate(2);
    if (result.key()!=0)
    {
        errout("SSL Client : Cannot Create SSL Context.ErrCode=%d.\n",result.key());
        return false;
    }

    ctx = result.value();
    return true;
}

int testTcpClient()
{
    base::aSSLContextCreator creator;
    base::aSSLContext asslctx;
    std::shared_ptr<network::EventLoop> eventLoop = std::make_shared<network::EventLoop>(4); // 4个线程
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::SSLStream> link;
    bool bret;

    // 开始事件循环
    bret = eventLoop->start();
    assert(bret);

    // config ssl
    bret = config_client_ssl(creator,asslctx);
    assert(bret);

    repoller = std::make_shared<network::RePoller>();
    link = std::make_shared<network::SSLStream>(base::Socket(AF_INET, SOCK_STREAM,0),repoller);

    link->usingSSL(asslctx.handle(), 2);

    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    std::weak_ptr<network::SSLStream> wlink = link;

    genout("SSL Client : Prepare To  Connect Server...\n");

    link->setOnClose([]()
    {
        m_exit= 1;
        errout("SSL Client : Disconnected , Prepare Exit...\n");
    });

    link->setOnWrite([wlink](ssize_t ret, std::string && data)
    {
        if (ret < 200)
        {
            genout("SSL Client : Write Some Data To Server Done...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("SSL Client : Write Some Data To Server Done...Size = [%ld]\t\r.",ret);
        }
        if(ret < 0)
        {
            auto link = wlink.lock();
            if (link)
            {
                link->aclose();
            }
        }
    });

    link->setOnRead([wlink](ssize_t ret, std::string && data)mutable
    {
        if (ret < 200)
        {
            genout("SSL Client : Read Some Data From Server, EchoBack ...Size = [%ld]. Content = [%s].\n",ret,data.c_str());
        }
        else
        {
            genout("SSL Client : Read Some Data From Server, EchoBack ...Size = [%ld]\t\r.",ret);
        }
        
        auto link = wlink.lock();
        if (link)
        {
            //link->asend(std::move(data)); // 返回true时，该函数会保存一份link强引用，直到send完成。
            link.reset();
        }
    });

    // 开始polling
    bret = repoller->start(eventLoop,5);
    assert(bret);

    /// 注意，要先polling才能asyncConnect
    bret = link->asyncConnect( serverip, serverport, [link, &promise](bool ok)
    {
        genout("SSL Client : OnConnected = %s.\n",(ok?"True":"False"));
        assert(ok);
        /// 需要SSL握手
        int ret = link->reference_sslio()->initiateHandShake();
        genout("SSL Client : SSL Initial HandShake ret = %d.\n",ret);
        assert(ret == 1);
        auto protocol = link->reference_sslio()->reference().selectedProtocolForClient();
        if( protocol.empty())
        {
            protocol = "null";
        }
        genout("SSL Client : Using Protocol `%s`. Size=%lu.\n", protocol.c_str(), protocol.size());
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
        errout("SSL Client : Connect Failed...Exit...\n");
        m_exit=1;
        goto end_clean;
    }

    for (int i =0; i< 10  && m_exit ==0; i++)
    {
        bret = link->asend("hello world-" + std::to_string(i) + " !!");
        assert(bret);
        genout("SSL Client : Send Some Data.\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
    genout ("SSL Client : Client Exit After 1000 msec...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    return 0;
}

int test_sslstream()
{
    pid_t pid = fork();

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

    if (pid != 0)
    { // server
        fprintf(stdout,"Parent PID %d.\n",::getpid());
        testTcpServer();
        ::kill(pid,SIGINT);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    else
    { // client
        fprintf(stdout,"Child PID %d.\n",::getpid());
        testTcpClient();
    }
    return 0;
}

int main(int argc , char ** argv)
{
    test_sslstream();
    return 0;
}

