
#include "aSSL.hh"

#include <asm-generic/errno-base.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "socket.hh"
#include "KVPair.hpp"

#include <openssl/ssl.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

namespace chrindex::andren::base
{
    void initSSLEnv()
    {
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
    }

    aSSLContextCreator::aSSLContextCreator(int endType): m_endType(endType) {}
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

    void aSSLContextCreator::setSupportedProtoForServer(std::vector<ProtocolsHandler> protoList)
    {
        for (auto &handler : protoList)
        {
            appendProtocolOnList(handler.protocol);
            m_protocbs[std::move(handler.protocol)] = std::move(handler.callback);
        }
        m_endType = 1;
    }

    void aSSLContextCreator::setSupportedProtoForClient(std::vector<ProtocolsHandler> protoList)
    {
        for (auto &handler : protoList)
        {
            appendProtocolOnList(handler.protocol);
            m_protocbs[std::move(handler.protocol)] = std::move(handler.callback);
        }
        m_endType = 2;
    }

    void aSSLContextCreator::setServerProxy_set_alpn_select_cb(alpn_select_cb_type cb)
    {
        m_set_alpn_select_cb = std::move(cb);
    }

    void aSSLContextCreator::setCallbackForLetMeDecideOn_select_next_protocol(alpn_select_cb_type cb)
    {
        m_set_alpn_select_decide_cb = std::move(cb);
       
    }

    KVPair<int, SSL_CTX *> aSSLContextCreator::startCreate(int method)
    {
        using CtxResult = KVPair<int,SSL_CTX *>;

        SSL_CTX *ctx = 0;
        std::string const &primaryKeyFile = m_primaryKeyFile;
        std::string const &certificateFile = m_certificateFile;

        auto errCtxFn = [](SSL_CTX *ctx)->SSL_CTX *
        {
            if (ctx!=0)
            {
                SSL_CTX_free(ctx);
            }
            return nullptr;
        };
        if(method==1)
        {
            ctx = SSL_CTX_new(TLS_server_method());
        }else if (method ==2)
        {
            ctx = SSL_CTX_new(TLS_client_method());
        }else 
        {
            return CtxResult(-2,nullptr);
        }
        
        if (!ctx)
        {
            // Could not create SSL/TLS context
            return CtxResult(-3,errCtxFn(ctx));
        }
        SSL_CTX_set_options(ctx, m_flags);
        static_assert(OPENSSL_VERSION_NUMBER >= 0x30000000L);
        if (SSL_CTX_set1_curves_list(ctx, "P-256") != 1)
        {
            // SSL_CTX_set1_curves_list failed
            return CtxResult(-4,errCtxFn(ctx));
        }
        if (SSL_CTX_use_PrivateKey_file(ctx, primaryKeyFile.c_str(), SSL_FILETYPE_PEM) != 1)
        {
            // Could not read private key file
            return CtxResult(-5,errCtxFn(ctx));
        }
        if (SSL_CTX_use_certificate_chain_file(ctx, certificateFile.c_str()) != 1)
        {
            // Could not read certificate file
            return CtxResult(-6,errCtxFn(ctx));
        }

        bool bret = false;
        if ( m_endType ==1)
        {
            bret = alpnForServer(ctx);
        }
        else if(m_endType ==2)
        {
            bret = alpnForClient(ctx);
        }else {
            // 不进行ALPN
        }
        if (!bret)
        {
            return CtxResult(-7,errCtxFn(ctx));
        }

        return CtxResult(0,ctx);
    }

    int aSSLContextCreator::select_next_protocol(unsigned char const **out, unsigned char *outlen,
                                                 unsigned char const *in, unsigned int inlen)
    {
        /// 如果设置了外部函数，则有外部回调函数决定是否协议握手成功
        if (m_set_alpn_select_decide_cb)
        {
            return m_set_alpn_select_decide_cb(out,outlen,in,inlen);
        }

        /// 否则由内部的方式决定是否握手成功

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


    /// @brief 服务端协议列表相关的设置
    /// @return 
    bool aSSLContextCreator::alpnForServer(SSL_CTX *ctx)
    {
        bool bret = false;
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
        bret = true;
#endif /* !OPENSSL_NO_NEXTPROTONEG */

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
        /// 客户端选定了哪个协议
        if (!m_set_alpn_select_cb)
        {
            SSL_CTX_set_alpn_select_cb(
                ctx, [](SSL *ssl, unsigned char const **out, unsigned char *outlen, unsigned char const *in, unsigned int inlen, void *arg) -> int
                {
                        int rv;
                        (void)ssl;
                        auto pself = reinterpret_cast<aSSLContextCreator *>(arg);
                        rv = pself->select_next_protocol(out, outlen, in, inlen);
                        
                        if (rv != 0)
                        {
                            return SSL_TLSEXT_ERR_NOACK;
                        }
                        return SSL_TLSEXT_ERR_OK; 
                },
                this);
        }
        else 
        {
            SSL_CTX_set_alpn_select_cb(ctx, [](SSL *ssl, unsigned char const **out, unsigned char *outlen, unsigned char const *in, unsigned int inlen, void *arg)->int
                {
                    auto pself = reinterpret_cast<aSSLContextCreator *>(arg);
                    return pself->m_set_alpn_select_cb(out, outlen, in, inlen);
                },this);
        }
        bret = true;
#endif /* OPENSSL_VERSION_NUMBER >= 0x10002000L */

        return bret;
    }

    /// @brief 客户端协议列表相关的设置
    /// @return 
    bool aSSLContextCreator::alpnForClient(SSL_CTX *ctx)
    {
        int ret = SSL_CTX_set_alpn_protos(ctx, reinterpret_cast<unsigned char*>(&m_next_proto_list[0]) , m_next_proto_list.size());
        return ret == 0 ;
    }


    void aSSLContextCreator::appendProtocolOnList(std::string const & protocol)
    {
        /**
         * 形如 ： unsigned char const list [] = 
         * { 
         *      8, 'h', 't', 't', 'p', '/', '1', '.', '1' ,
         *      3, 'f' , 't' , 'p'
         * };
         * 
         */
        std::string tmp;
        unsigned char size = static_cast<unsigned char>(protocol.size());

        tmp.resize(size + 1);
        tmp[0] = size;
        memcpy(&tmp[1],&protocol[0],size);
        
        m_next_proto_list.append(tmp);
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

    bool aSSLContext::valid()const {return m_ctx!=0;}

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

    int aSSL::getSSLError(int ret)const
    {
        return SSL_get_error(m_ssl,ret);
    }

    bool aSSL::valid() const { return m_ssl != 0; }

    ssl_st *aSSL::handle() const { return m_ssl; }

    ssl_st *aSSL::take_ssl()
    {
        auto p = m_ssl;
        m_ssl = 0;
        return p;
    }

    std::string aSSL::selectedProtocolForClient() 
    {
        std::string protocol;
        unsigned char const * str = 0;
        unsigned int len = 0;
        SSL_get0_alpn_selected(handle(), &str, &len);

        if (len != 0 && str !=0)
        {
            protocol.resize(len);
            memcpy(&protocol[0],str,len);
        }
        return protocol;
    }
    

    aSSLSocketIO::aSSLSocketIO() :m_endType(0) {}

    aSSLSocketIO::aSSLSocketIO(aSSLSocketIO &&_)
    {
        m_ssl = std::move(_.m_ssl);
        m_endType = std::move(_.m_endType);
    }
    aSSLSocketIO::~aSSLSocketIO()
    {
        shutdown();
    }

    void aSSLSocketIO::operator=(aSSLSocketIO &&_)
    {
        this->~aSSLSocketIO();
        m_ssl = std::move(_.m_ssl);
        m_endType = std::move(_.m_endType);
    }

    void aSSLSocketIO::upgradeFromSSL(aSSL &&ssl)
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

    int aSSLSocketIO::initiateHandShake()
    {
        return SSL_connect(m_ssl.handle());
    }

    KVPair<ssize_t, std::string> aSSLSocketIO::read()
    {
        ssize_t size;
        std::string buffer;

        // 让SSL调用::recv但是没法存到用户缓冲区，这样SSL_pending就是要分配缓冲区的大小。
        size = SSL_read(m_ssl.handle(), 0,0);

        size = SSL_pending(m_ssl.handle());

        if (size <= 0 )
        {
            return {-1, {}};
        }
        buffer.resize(size);
        size = SSL_read(m_ssl.handle(), &buffer[0], buffer.size());

        return {buffer.size(), std::move(buffer)};
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
        if (m_ssl.valid())
        {
            return SSL_shutdown(m_ssl.take_ssl());
        }
        return 0;
    }

    bool aSSLSocketIO::valid() const 
    {
        return m_ssl.valid();
    }

    int aSSLSocketIO::endType() const
    {
        return m_endType;
    }

    void aSSLSocketIO::setEndType(int end_type)
    {
        m_endType = end_type;
    }

    bool aSSLSocketIO::enableNonBlock(bool enabled)
    {
        int fd = SSL_get_fd(m_ssl.handle()) ;
        if (fd > 0)
        {
            int flags = ::fcntl(fd, F_GETFL, 0);
            if (enabled) { flags |=  O_NONBLOCK; }
            else { flags &=~O_NONBLOCK; }
            return 0 == ::fcntl(fd, F_SETFL, flags );
        }
        return false;
    }

}
