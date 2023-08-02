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
        aSSLContextCreator();
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

        void setSupportedProto(std::vector<ProtocolsHandler> protoList);

        SSL_CTX *startCreate();

    private:
        int select_next_protocol(unsigned char **out, unsigned char *outlen,
                                 unsigned char const *in, unsigned int inlen);

    private:
        std::string m_primaryKeyFile;
        std::string m_certificateFile;
        int m_flags;
        std::string m_next_proto_list;
        std::unordered_map<std::string, std::function<bool()>> m_protocbs;
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

        bool valid() const;

        ssl_st *handle() const;

        ssl_st *take_ssl();

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
        void upgradeFromSSL(aSSL ssl);

        /// @brief 获得内部aSSL实例的引用。
        /// 不应该也不建议转移该函数返回aSSL实例引用的的所有权。
        /// @return
        aSSL &reference();

        int bindSocketFD(Socket const &sock);

        int handShake();

        KVPair<ssize_t, std::string> read();

        ssize_t write(std::string const &data);

        ssize_t write(char const * data, size_t lenght);

        int shutdown();

    private:
        aSSL m_ssl;
    };

}
