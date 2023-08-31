

#include "../src/network/andren_network.hh"
#include <memory>

using namespace chrindex::andren;


#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit;

std::string serverip = "192.168.88.3";
int32_t serverport = 8080;

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
                    genout("HTTPS Server : SSL Client Support Protocol `h2`.\n");
                    return true;
                }
            },
            {
                "h2c", []() -> bool
                {
                    genout("HTTPS Server : SSL Client Support Protocol `h2c`.\n");
                    return true;
                }
            }
        });

    creator.setCallbackForLetMeDecideOn_select_next_protocol([](
        unsigned char const **out, unsigned char *outlen, 
        unsigned char const *in, unsigned int inlen)->int
    {
        auto rv = network::proxy_nghttp2_select_next_protocol(const_cast<unsigned char **>(out), outlen, in, inlen);

        std::string usedP(reinterpret_cast<char const*>(out[0]), *outlen);
        genout("HTTPS Server : SSL CTX : nghttp2 selected next protocol ret = %d."
            " Using Protocol [%s].\n", rv , usedP.c_str() );

        if (rv != 1) {
            return SSL_TLSEXT_ERR_NOACK;
        }

        return SSL_TLSEXT_ERR_OK;
    });
    
    if (auto pctx = creator.startCreate(1); pctx.key()==0)
    {
        ctx = pctx.value();
        genout("HTTPS Server : SSL Create OpenSSL Context Done.\n");
    }
    else
    {
        errout("HTTPS Server : SSL Create OpenSSL Context Failed. ErrCode=%d.\n",pctx.key());
        return false;
    }
    return true;
}


int test_http2server()
{
    base::aSSLContextCreator creator;
    base::aSSLContext sslctx;
    std::shared_ptr<network::EventLoop> eventLoop = std::make_shared<network::EventLoop>(4); // 4个线程
    std::shared_ptr<network::RePoller> repoller;
    std::shared_ptr<network::Acceptor> acceptor;
    base::Socket ssock(AF_INET, SOCK_STREAM,0);
    base::EndPointIPV4 epv4(serverip,serverport);
    int ret [[maybe_unused]] =0;
    bool bret [[maybe_unused]] =false;

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

    genout("HTTPS Server : Start Listen And Prepare Accept...\n");

    acceptor->setOnAccept([ &sslctx ](std::shared_ptr<network::SockStream> raw_link)
    {
        if(!raw_link) 
        {
            m_exit= 1;
            errout("HTTPS Server : Accept Faild! Socket Error.\n");
            return ;
        }
        int fd = raw_link->reference_handle()->handle();

        std::shared_ptr<network::Http2ndSession> session 
            = std::make_shared<network::Http2ndSession>(1, std::move(std::move(*raw_link)));

        std::weak_ptr<network::Http2ndSession> wsession = session; 
        std::weak_ptr<network::RePoller> wrepoller = session->streamReference()->reference_repoller();
        std::shared_ptr<network::RePoller> repoller = wrepoller.lock();

        /// Configurate OpenSSL 
        base::aSSL assl = createSSLFromCtx(sslctx); 
        bool bret [[maybe_unused]] = session->streamReference()->usingSSL(std::move(assl), 1);
        assert(bret);

        /// 握手
        bret = session->tryHandShakeAndInit();
        assert(bret);

        session->setOnCallback({

            // OnReqRecvDone
            [](network::Http2ndStream * , network::Http2ndSession *)->int
            {
                genout("HTTPS Server : Request Recv Done.\n");
                return 0;
            },

            // OnStreamClosed
            [](network::Http2ndStream *, uint32_t errcode , network::Http2ndSession *)->int
            {
                genout("HTTPS Server : Stream Closed.\n");
                return 0;
            },

            // OnHeadDone
            [](network::Http2ndStream * stream , network::Http2ndSession *)->int 
            {
                genout("HTTPS Server : Head Done.REQ = [%s].\n", 
                    stream->referent_request()->headers[":path"].c_str());
                return 0;
            },

            // OnNewHead
            [](network::Http2ndStream * , network::Http2ndSession *)->int 
            {
                genout("HTTPS Server : New Head Done.\n");
                return 0;
            },

            // OnDataChunkRecv
            [](network::Http2ndStream * ,uint8_t flags, std::string && data , network::Http2ndSession *)->int 
            {
                genout("HTTPS Server : Data Recv : [%s].\n", data.c_str());
                return 0;
            },

            // OnPushPromiseFrame
            [](network::Http2ndStream * , network::Http2ndSession *, network::Http2ndFrame::push_promise const &)->int 
            {
                genout("HTTPS Server : Push Promise.\n");
                return 0;
            },

            // OnSessionClose
            [wrepoller,fd]()
            {
                std::shared_ptr<network::RePoller> repoller = wrepoller.lock();
                if (!repoller)
                {
                    return ;
                }

                ///s 删除对象
                repoller->findObject(fd, true , [ ](bool ret, std::any * obj)
                {
                    genout("HTTPS Server : This Client Is TakeOut From The ObjectsMap.\n");
                });
                return ;
            }
        });


        /// 对象托管
        /// 反正用到的时候可以查
        bret = repoller->saveObject(fd, true, session, [](bool ret, std::any * obj)
        {
            genout("HTTPS Server : This Session Is Saved Into The ObjectsMap.\n");
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
    genout ("HTTPS Server: Exit...\n");
    return 0;
}


int testHttp2()
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

    test_http2server();

    return 0;
}



int main(int argc , char ** argv)
{
    testHttp2();

    return 0;
}


