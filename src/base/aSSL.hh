#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string>
#include <vector>
#include <functional>

#include "KVPair.hpp"
#include "socket.hh"

namespace chrindex::andren::base
{
    /// @brief 初始化OpenSSL环境。仅需要调用一次。
    void initSSLEnv();

    /// @brief SSL上下文配置创建类
    class aSSLContextCreator
    {
    public:
        /// @brief 
        /// @param endType 默认是0，即不适用ALPN；1-服务端；2-客户端。
        /// 请注意，该设置会被setSupportedProtoForServer 函数或者
        /// setSupportedProtoForClient 函数覆盖。
        aSSLContextCreator(int endType=0);
        ~aSSLContextCreator();

        struct ProtocolsHandler
        {
            ProtocolsHandler() {}
            ProtocolsHandler(std::string const &_protocol, std::function<bool()> _callback)
                : protocol(_protocol), callback(_callback) {}
            ~ProtocolsHandler() {}

            std::string protocol;
            std::function<bool()> callback;
        };

        aSSLContextCreator &setPrimaryKeyFilePath(std::string const &primaryKeyFile);

        aSSLContextCreator &setCertificateFileFilePath(std::string const &certificateFile);

        aSSLContextCreator &setOptionsFlags(int flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                                                        SSL_OP_NO_COMPRESSION |
                                                        SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

        //// #######################  APLN #####################
        //// ALPN（Application-Layer Protocol Negotiation）即应用层协议协商。
        //// ALPN是可选的。主动进行ALPN的意义是让OpenSSL在握手时顺便帮助服务端和
        //// 客户端程序进行应用层协议协商，而不是自己再整一套协议协商的方式。
        //// SPL（Server's Protocol List）即服务端协议列表。同理还有CPL。
        //// 当客户端发起握手时，OpenSSL尝试进行ALPN协商，这时客户端会发送CPL到
        //// 服务端，然后服务端与SPL进行相交，得出交集。如果该交集为空集，则服务端
        //// 和客户端没有共同支持的应用层协议，即ALPN中止，两者会继续使用默认的TLS
        //// 进行通信。如果交集不为空，该交集内的所有协议，被以回调函数参数的形式
        //// 传递到服务端的协议选中函数内。通常根据服务端设置的选择函数的顺序，排前
        //// 支持的会被选中，然后发回该选择到客户端。最后客户端对应的协议选中函数内
        //// 得到服务端的选择结果，并根据该结果进行资源初始化等操作。
        //// 总结来说，ALPN的大致过程是：
        ////    1.客户端提供了支持的协议列表CPL。
        ////    2.服务端提供了支持的协议列表SPL。
        ////    3.SPL和CPL的交集非空。
        ////    4.回调函数（即协议选择函数）的参数是交集部分，用于服务端从CPL中选择一个协议进行返回给客户端。

        /// @brief 为ALPN设置SPL。
        /// @param protoList SPL列表以及每个Protocol被选中时的回调函数。
        void setSupportedProtoForServer(std::vector<ProtocolsHandler> protoList);

        /// @brief 为ALPN设置CPL。
        /// @param protoList CPL列表以及每个Protocol被选中时的回调函数。
        void setSupportedProtoForClient(std::vector<ProtocolsHandler> protoList);

        /// @brief 
        /// @return 
        
        /// @brief 开始创建SSL Context
        /// @param method 1为Server Method ， 2为Client Method
        /// @return <ret , value> 失败时value = nullptr， 且ret非0。
         KVPair<int,SSL_CTX *> startCreate(int method);

    private:

        /// @brief 将protocol转化为line-format并追加到buffer
        /// @param protocol 
        void appendProtocolOnList(std::string const & protocol);

        /// @brief 服务端协议列表相关的设置
        /// @return 
        bool alpnForServer(SSL_CTX *ctx);

        /// @brief 客户端协议列表相关的设置
        /// @return 
        bool alpnForClient(SSL_CTX *ctx);

        /// @brief OpenSSL回调用来去协议列表的。
        /// @param out 
        /// @param outlen 
        /// @param in 
        /// @param inlen 
        /// @return 
        int select_next_protocol(unsigned char const **out, unsigned char *outlen,
                                 unsigned char const *in, unsigned int inlen);

    private:
        std::string m_primaryKeyFile;
        std::string m_certificateFile;
        int m_flags;
        std::string m_next_proto_list;
        std::unordered_map<std::string, std::function<bool()>> m_protocbs;
        int m_endType; // 服务端还是客户端
    };

    /// @brief  SSL上下文配置类
    class aSSLContext
    {
    public:
        aSSLContext();
        aSSLContext(SSL_CTX *ctx);
        aSSLContext(aSSLContext &&_);
        ~aSSLContext();
        void operator=(aSSLContext &&_);

        SSL_CTX *handle() const;

        SSL_CTX *take();

        bool valid()const ;

    private:
        SSL_CTX *m_ctx;
    };

    /// @brief SSL实例类
    class aSSL
    {
    public:
        aSSL();
        aSSL(SSL_CTX *ssl_ctx);
        aSSL(ssl_st *_ssl);
        aSSL(aSSL &&_);

        virtual ~aSSL();

        void operator=(aSSL &&_);

        static unsigned long getErrNo();

        static std::string sslErrString();

        int getSSLError(int ret)const; 

        bool valid() const;

        ssl_st *handle() const;

        ssl_st *take_ssl();

        /// @brief 查询服务端选定的应用层协议
        /// 该函数应该在完成握手后客户端主动调用。
        /// @return 协议名，否则空。
        std::string selectedProtocolForClient();

    private:
        ssl_st *m_ssl;
    };

    /// @brief 由SSL实例和socket文件描述符升格成的SSL输入输出类
    class aSSLSocketIO
    {
    public:
        aSSLSocketIO();
        aSSLSocketIO(aSSLSocketIO &&_);
        ~aSSLSocketIO();

        void operator=(aSSLSocketIO &&_);

        /// @brief SSL升格为Socket SSL对象。
        /// 该函数会转移aSSL实例的所有权。
        /// @param ssl 被升格的aSSL实例。
        void upgradeFromSSL(aSSL && ssl);

        /// @brief 获得内部aSSL实例的引用。
        /// 不应该也不建议转移该函数返回aSSL实例引用的对象所有权。
        /// @return
        aSSL &reference();

        /// @brief 提供用于处理IO的Socket FD
        /// 请注意，不要使用int fd进行构造。
        /// 否则fd会因为临时对象的生命周期而被close。
        /// @param sock 
        /// @return 
        int bindSocketFD(Socket const &sock);
        
        /// @brief （服务端）同意进行一次SSL/TLS握手
        /// @return 
        int handShake();

        /// @brief （客户端）发起一次握手
        /// @return 
        int initiateHandShake();

        /// @brief 读数据。读出来的数据是已经解密的
        /// @return 
        KVPair<ssize_t, std::string> read();

        /// @brief 写数据
        /// @param data 
        /// @return 
        ssize_t write(std::string const &data);

        /// @brief 写数据
        /// @param data 
        /// @param lenght 
        /// @return 
        ssize_t write(char const * data, size_t lenght);

        /// @brief 停止SSL IO
        /// @return 
        int shutdown();

        bool valid() const ;

        /// @brief SSL端类型
        /// @return 1是服务端，2则是客户端 
        int endType() const;

        /// @brief 设置SSL端类型
        /// @param end_type 设置为1是服务端，2则是客户端
        void setEndType(int end_type);

    private:
        aSSL m_ssl;
        int m_endType;
    };

}
