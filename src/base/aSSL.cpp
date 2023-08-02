
#include "aSSL.hh"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "socket.hh"
#include "KVPair.hpp"

namespace chrindex::andren::base
{
    void initSSLEnv()
    {
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
    }

    aSSLContextCreator::aSSLContextCreator() {}
    aSSLContextCreator::~aSSLContextCreator() {}

    aSSLContextCreator &aSSLContextCreator::setPrimaryKeyFilePath(std::string const &primaryKeyFile)
    {
        m_primaryKeyFile = primaryKeyFile;
        return *this;
    }

    aSSLContextCreator &aSSLContextCreator::setCertificateFileFilePath(std::string const &certificateFile)
    {
        m_certificateFile = certificateFile;
        return *this;
    }

    aSSLContextCreator &aSSLContextCreator::setOptionsFlags(int flags)
    {
        m_flags = flags;
        return *this;
    }

    void aSSLContextCreator::setSupportedProto(std::vector<ProtocolsHandler> protoList)
    {
        for (auto &handler : protoList)
        {
            m_protocbs[std::move(handler.protocol)] = std::move(handler.callback);
        }
    }

    SSL_CTX *aSSLContextCreator::startCreate()
    {
        SSL_CTX *ctx = 0;
        std::string const &primaryKeyFile = m_primaryKeyFile;
        std::string const &certificateFile = m_certificateFile;

        ctx = SSL_CTX_new(TLS_server_method());
        if (!ctx)
        {
            // Could not create SSL/TLS context
            return 0;
        }
        SSL_CTX_set_options(ctx, m_flags);
        static_assert(OPENSSL_VERSION_NUMBER >= 0x30000000L);
        if (SSL_CTX_set1_curves_list(ctx, "P-256") != 1)
        {
            // SSL_CTX_set1_curves_list failed
            return 0;
        }
        if (SSL_CTX_use_PrivateKey_file(ctx, primaryKeyFile.c_str(), SSL_FILETYPE_PEM) != 1)
        {
            // Could not read private key file
            return 0;
        }
        if (SSL_CTX_use_certificate_chain_file(ctx, certificateFile.c_str()) != 1)
        {
            // Could not read certificate file
            return 0;
        }
#ifndef OPENSSL_NO_NEXTPROTONEG
        /// 设置`服务器支持什么协议`回调
        /// 当 TLS 服务器需要下一个协议协商支持的协议列表时调用该回调
        SSL_CTX_set_next_protos_advertised_cb(
            ctx,
            [](SSL *ssl, unsigned char const **data, unsigned int *len, void *arg) -> int
            {
                (void)ssl; // unuse
                auto pself = reinterpret_cast<aSSLContextCreator *>(arg);
                *data = reinterpret_cast<unsigned char *>(&(pself->m_next_proto_list[0]));
                *len = pself->m_next_proto_list.size();
                return SSL_TLSEXT_ERR_OK;
            },
            this);
#endif /* !OPENSSL_NO_NEXTPROTONEG */

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
        /// 客户端选定了哪个协议
        SSL_CTX_set_alpn_select_cb(
            ctx, [](SSL *ssl, unsigned char const **out, unsigned char *outlen, unsigned char const *in, unsigned int inlen, void *arg) -> int
            {
                    int rv;
                    (void)ssl;
                    auto pself = reinterpret_cast<aSSLContextCreator *>(arg);
                    rv = pself->select_next_protocol((unsigned char **)out, outlen, in, inlen);
                    if (rv != 1)
                    {
                        return SSL_TLSEXT_ERR_NOACK;
                    }
                    return SSL_TLSEXT_ERR_OK; },
            this);
#endif /* OPENSSL_VERSION_NUMBER >= 0x10002000L */
        return ctx;
    }

    int aSSLContextCreator::select_next_protocol(unsigned char **out, unsigned char *outlen,
                                                 unsigned char const *in, unsigned int inlen)
    {
        std::string key;
        unsigned int i = 0;

        *out = 0;
        *outlen = 0;

        while (i < inlen && in[i] > 0)
        {
            int size = in[i];
            char const *p = reinterpret_cast<char const *>(&in[i + 1]);

            i += (size + 1); // migration to next protocol's size of bytes

            key.resize(size);
            memcpy(&key[0], p, size);

            auto iter = m_protocbs.find(key);
            if (iter != m_protocbs.end())
            {
                if (iter->second)
                {
                    bool bret = iter->second();
                    if (bret)
                    {
                        *out = reinterpret_cast<unsigned char *>(const_cast<char *>(p));
                        *outlen = size;
                    }
                    return bret ? 0 : -2; // `OK` or `callback function tell me initialize failed.`
                }
            }
            else
            {
                return -3; // support but non set callback.
            }
        }
        return -1; // no support
    }

    aSSLContext::aSSLContext() : m_ctx(0) {}
    aSSLContext::aSSLContext(SSL_CTX *ctx) : m_ctx(ctx) {}
    aSSLContext::aSSLContext(aSSLContext &&_)
    {
        m_ctx = _.m_ctx;
        _.m_ctx = 0;
    }
    aSSLContext::~aSSLContext()
    {
        if (m_ctx)
        {
            SSL_CTX_free(m_ctx);
        }
    }
    void aSSLContext::operator=(aSSLContext &&_)
    {
        this->~aSSLContext();
        m_ctx = _.m_ctx;
        _.m_ctx = 0;
    }

    SSL_CTX *aSSLContext::handle() const { return m_ctx; }

    SSL_CTX *aSSLContext::take()
    {
        SSL_CTX *p = m_ctx;
        m_ctx = 0;
        return p;
    }

    aSSL::aSSL() : m_ssl(0) {}
    aSSL::aSSL(SSL_CTX *ssl_ctx)
    {
        SSL *ssl;
        ssl = SSL_new(ssl_ctx);
        m_ssl = ssl;
    }
    aSSL::aSSL(ssl_st *_ssl) : m_ssl(_ssl) {}
    aSSL::aSSL(aSSL &&_)
    {
        m_ssl = _.m_ssl;
        _.m_ssl = 0;
    }

    aSSL::~aSSL()
    {
        if (m_ssl)
        {
            SSL_free(m_ssl);
        }
    }

    void aSSL::operator=(aSSL &&_)
    {
        this->~aSSL();
        m_ssl = _.m_ssl;
        _.m_ssl = 0;
    }

    unsigned long aSSL::getErrNo()
    {
        return ERR_get_error();
    }

    std::string aSSL::sslErrString()
    {
        return ERR_error_string(getErrNo(), NULL);
    }

    bool aSSL::valid() const { return m_ssl != 0; }

    ssl_st *aSSL::handle() const { return m_ssl; }

    ssl_st *aSSL::take_ssl()
    {
        auto p = m_ssl;
        m_ssl = 0;
        return p;
    }

    aSSLSocketIO::aSSLSocketIO() {}

    aSSLSocketIO::aSSLSocketIO(aSSLSocketIO &&_)
    {
        m_ssl = std::move(_.m_ssl);
    }
    aSSLSocketIO::~aSSLSocketIO()
    {
        if (m_ssl.valid())
        {
            shutdown();
        }
    }

    void aSSLSocketIO::operator=(aSSLSocketIO &&_)
    {
        this->~aSSLSocketIO();
        m_ssl = std::move(_.m_ssl);
    }

    void aSSLSocketIO::upgradeFromSSL(aSSL ssl)
    {
        m_ssl = std::move(ssl);
    }

    aSSL &aSSLSocketIO::reference()
    {
        return m_ssl;
    }

    int aSSLSocketIO::bindSocketFD(Socket const &sock)
    {
        return SSL_set_fd(m_ssl.handle(), sock.handle());
    }

    int aSSLSocketIO::handShake()
    {
        return SSL_accept(m_ssl.handle());
    }

    KVPair<ssize_t, std::string> aSSLSocketIO::read()
    {
        ssize_t size;
        std::string buffer;

        size = SSL_pending(m_ssl.handle());
        if (size <= 0)
        {
            return {std::move(size), {}};
        }
        buffer.resize(size);
        size = SSL_read(m_ssl.handle(), &buffer[0], buffer.size());
        return {std::move(size), std::move(buffer)};
    }

    ssize_t aSSLSocketIO::write(std::string const &data)
    {
        return this->write(data.c_str(), data.size());
    }

    ssize_t aSSLSocketIO::write(char const * data, size_t lenght)
    {
        return SSL_write(m_ssl.handle(), data, lenght);
    }

    int aSSLSocketIO::shutdown()
    {
        return SSL_shutdown(m_ssl.handle());
    }

}
