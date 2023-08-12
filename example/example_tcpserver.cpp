
#include "../src/network/andren_network.hh"


using namespace chrindex::andren;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit;

std::string serverip = "192.168.88.3";
int32_t serverport = 8317;

int testTcpServer()
{
    genout("TCP Server : Initial...\n");
    
    std::shared_ptr<network::EventLoop> ev = std::make_shared<network::EventLoop>(4);
    std::shared_ptr<network::ProPoller> pp = std::make_shared<network::ProPoller>();
    std::shared_ptr<network::TcpStreamManager> tsm = std::make_shared<network::TcpStreamManager>();

    ev->start();
    pp->start(ev);

    tsm->setEventLoop(ev);
    tsm->setProPoller(pp);
    auto errcode = tsm->start(serverip,serverport);
    if (errcode != network::TSM_Error::OK)
    {
        errout("TcpServer : Start Listen Failed.\n");
        return -1;
    }

    network::TSM_Error tsme = tsm->requestAccept([](std::shared_ptr<network::TcpStream> cstream, network::TSM_Error errcode)
    {
        if (errcode == network::TSM_Error::ACCEPT_FAILED)
        {
            errout ("TcpServer : Cannot Accept.\n");
            return ;
        }   
        if (errcode == network::TSM_Error::ACCEPT_TIMEOUT)
        {
            errout ("TcpServer : Accept Timeout. Retry...\n");
            return ;
        }
        if (errcode != network::TSM_Error::OK || !cstream)
        {
            errout("TcpServer : Other Error.\n");
            return ;
        }

        sockaddr_storage ss;
        uint32_t len;
        cstream->handle()->getpeername(reinterpret_cast<sockaddr*>(&ss),&len);

        if (len != base::EndPointIPV4::addrSize())
        {
            errout ("TcpServer : Error! Only Support IPV4.... Disconnect.\n");
            return ;
        }

        base::EndPointIPV4 epv4(reinterpret_cast<sockaddr_in*>(&ss));
        std::string ip = epv4.ip();
        int32_t port = epv4.port();

        cstream->setOnClose([cstream, ip , port]()
        {
            // 无需手动close
            // 不过要注意cstream的清理。
            errout ("TcpServer : Client [%s:%d] Disconnected.\n" , ip.c_str(), port);
        });

        cstream->reqRead([cstream](ssize_t ret, std::string && data)
        {
            /// 该函数的ret绝不等于0。
            /// 等于0的时候已经回调OnClose了。

            if (ret < 0 ) // timeout
            {
                errout ("TcpServer : Client Request Read Data Timeout.ret=%ld.\n",ret);
                return ;
            }

            genout ("TcpServer : Client Data Done! Size = %ld , Data = [%s].\n", ret, data.c_str());
            genout ("TcpServer : Prepare Request Echo Data Back.\n");
            
            cstream->reqWrite(std::move(data) , [cstream](ssize_t ret, std::string && lastData)
            {
                genout ("TcpServer : Request Echo Data Back.\n");
            });
        },  
        5000);
    });

    if (tsme != network::TSM_Error::OK)
    {
        errout("TcpServer : Cannot Create A New Accept Request.\n");
        m_exit =1;
    }

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    genout ("TcpServer : Server Exit...\n");

    return 0;
}

int testTcpClient()
{
    genout("TCP Client : Wait...\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 等Server进程一会

    genout("TCP Client : Initial...\n");

    std::shared_ptr<network::EventLoop> ev = std::make_shared<network::EventLoop>(4);
    std::shared_ptr<network::ProPoller> pp = std::make_shared<network::ProPoller>();
    std::shared_ptr<network::TcpStream> cstream = std::make_shared<network::TcpStream>( base::Socket(AF_INET, SOCK_STREAM, 0) );

    bool bret ;

    bret = ev->start();
    assert(bret);

    bret = pp->start(ev);
    assert(bret);

    cstream->setEventLoop(ev);

    if (!cstream->valid())
    {
        errout("TCP Client : Cannot Socket A New FD.\n");
        return -1;
    }

    cstream->setOnClose([]()
    {
        genout("TCP Client : Client Disconnected...\n");
        m_exit = 1;
    });

    // windows host 
    // serverip = "192.168.88.2";
    // serverport = 8317;

    bret = cstream->reqConnect(serverip,serverport,[pp,cstream](int status)
    {
        if(status != 0) // Cannot Connected
        {
            errout ("TCP Client : Cannot Create A New Connection.\n");
            m_exit = 1;
            return ;
        }
        /// connected

        bool bret = cstream->setProPoller(pp);// 设置poller并加回Read事件

        assert(bret);

        genout("TCP Client : Connected!! Prepare Write Data.\n");

        std::string message = "Hello World!  ------ Love From Client.";

        bret = cstream->reqWrite(message,[cstream](ssize_t ret, std::string && data)
        {
            if (ret <= 0)
            {
                errout ("TCP Client : Cannot Send A New Message.\n");
                return ;
            }

            genout("TCP Client : Send Data [%s] .\n",data.c_str());
            genout("TCP Client : Prepare A Read Data Request.\n");

            bool bret = cstream->reqRead([cstream](ssize_t ret, std::string && data)
            {
                if (ret < 0 ) // timeout
                {
                    errout ("TCP Client : Client Request Read Data Timeout Or Error. ret = %ld.\n",ret);
                    cstream->disconnect();
                }
                else
                {
                    genout ("TCP Client : Data Done! Size = %ld , Data = [%s].\n", ret, data.c_str());
                } 
                cstream->disconnect();
                genout("TCP Client : Try Disconnect.\n");
            },
            10000);

            if(!bret) // Cannot Connected
            {
                errout ("TCP Client : Cannot Reqeust To Read A New Message.\n");
                m_exit = 1;
                return ;
            }
        });
        if(!bret) // Cannot Connected
        {
            errout ("TCP Client : Cannot Reqeust To Send A New Message.\n");
            m_exit = 1;
            return ;
        }
    });

    if (!bret)
    {
        errout ("TCP Client : Cannot Request To Create A New Connection.\n");
        return -2;
    }

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
    int pid = fork();

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

    if (pid ==0)
    {
        testTcpClient();
    }else 
    {
        testTcpServer();
    }

    while(1)
    {
        if (m_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}



int main()
{
    testTCPServerAndClient();
    return 0;
}





