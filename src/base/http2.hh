#pragma once

#include <sys/socket.h>
#include <fcntl.h>


#include <nghttp2/nghttp2.h>
#include <nghttp2/nghttp2ver.h>

#include <string>
#include <stdint.h>
#include <memory>
#include <map>
#include <optional>
#include <stdexcept>
#include <any>

#include "exception.hh"
#include "socket.hh"
#include "aSSL.hh"

namespace chrindex::andren::base
{
    struct Http2StreamData
    {
        std::string requestPath;
        std::string data;
    };

    class Http2Stream
    {
    public:
        Http2Stream() {}
        Http2Stream(Http2Stream &&_) {}
        ~Http2Stream() {}

        Http2StreamData * createStreamData(int streamId)
        {
            return &m_root[streamId];
        }

        std::optional<Http2StreamData*> referenceStreamData(int32_t streamId)
        {
            if (auto iter = m_root.find(streamId); iter != m_root.end())
            {
                return &iter->second;
            }
            return {};
        }

        std::optional<Http2StreamData> takeOne(int32_t streamId)
        {
            if (auto iter = m_root.find(streamId); iter != m_root.end())
            {
                auto && s = std::move(iter->second);
                m_root.erase(iter);
                return s;
            }
            return {};
        }

        ssize_t count() const
        {
            return m_root.size();
        }

        void closeOne(int32_t streamId)
        {
            m_root.erase(streamId);
        }

        void closeAll()
        {
            m_root.clear();
        }

    private:
        // <streamId , stream >
        std::map<int32_t,Http2StreamData> m_root;
    };

    class Http2rdSession
    {
    public:
        Http2rdSession() {}

        Http2rdSession(std::unique_ptr<aSSLSocketIO> ssl)
        {
            _data = std::make_unique<Data>();
            _data->m_ssl = std::move(ssl);
           _CreateHttp2rdSession();
        }

        Http2rdSession(std::shared_ptr<Socket> sock)
        {
            _data = std::make_unique<Data>();
            _data->m_sock = std::move(sock);
            _CreateHttp2rdSession();
        }

        Http2rdSession(Http2rdSession &&_)
        {
            _data = std::move(_._data);
        }
        ~Http2rdSession()
        {
            if (_data)
            {
                nghttp2_session_del(_data->m_session);
                if (_data->m_ssl){
                    _data->m_ssl->shutdown();
                }
                if (_data->m_sock){
                    _data->m_sock->shutdown(SHUT_RDWR);
                }
            }
        }

        void operator=(Http2rdSession &&_)
        {
            this->~Http2rdSession();
            _data = std::move(_._data);
        }

        bool valid() const { return _data != nullptr; }

        /// @brief 该函数尝试以非阻塞的方式接收数据到NGHTTP的内部缓冲区。
        /// 通常需要配合epoll、select、poll或者io_uring使用。
        /// @return  0 == OK , 
        /// -1 == Socket or SSL-IO disconnected/closed , 
        /// -2 == nghttp2 failed to recv,
        /// -3 == nghttp2 send and finished frame error  , 
        /// -4 == SSL interface invoked error , 
        /// -5 == Socket Error。
        /// 通常-4，可以进一步使用SSL的error接口查看进一步的情况，
        /// -3 也是同理。
        /// -1 和 -2 代表不可恢复的错误，这时候会主动断开连接。
        /// -5 Socket的read接口错误（返回-1），函数会处理非阻塞时
        /// (MSG_DONTWAIT或者O_NONBLOCK)接口无数据的情况，
        /// 且转而返回0，其他情况则是返回-5；用户需要处理其他错误。
        /// 通过errno可以获取该错误。
        int tryRecvIntoTheHttpSession()
        {
            return recvAndStoreFromSocket();
        }
        
    private :

        void _CreateHttp2rdSession()
        {
            if(!_data){_data = std::make_unique<Data>();}

            nghttp2_session_callbacks *callbacks;
            nghttp2_session_callbacks_new(&callbacks);

            // 实际`发送数据`的回调函数
            // nghttp2_session_mem_send 和 nghttp2_send_callback 都是发数据的，
            // 一般来说时使用其中一个就够了。
            // 这里使用的是 nghttp2_send_callback 回调的方式发数据。
            // 也就是说让nghttp2自己决定什么时候发数据，而无需用户干涉。
            nghttp2_session_callbacks_set_send_callback(callbacks,
                [](nghttp2_session *session, const uint8_t *data,
                   size_t length, int flags, void *uptr)->ssize_t
                {
                    (void)session;
                    flags = 0; // NGHTTP2中暂未使用
                    ssize_t ret = 0;
                    auto self = coverFrom(uptr);
                    if(self->_data->m_ssl){
                        ret = self->_data->m_ssl->write( reinterpret_cast<char const*>(data) ,length);
                    }else if(self->_data->m_sock)
                    {
                        ret = self->_data->m_sock->send( reinterpret_cast<char const*>(data) ,length, flags);
                    }else {
                        throw aException(-1,"The current IO provider is not available。"); // IO不可用
                    }
                    return ret;
                });

            // `收到并处理数据帧`（包括头部帧、数据帧、优先级帧、设置帧等）的回调函数
            nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks,
                [](nghttp2_session *session, nghttp2_frame const *frame, void *uptr)->int
                {
                    // 对于server而言，客户端的一个HTTP请求可能会在传输时分成多个帧。
                    // 每拼凑足够一个完整帧，该函数会被调用。
                    // 该函数一般可以用于接收Content-Length所指示大小的Data。
                    // 当然也可以处理其他的事情，但个人觉得最好只拿来接收Data部分的数据。
                    // Data数据需要拼接，以满足Content-Length所指示的大小。
                    auto self = coverFrom(uptr);
                    if (self->_data->m_onFrameDone)
                    {
                        return self->_data->m_onFrameDone(*frame);
                    }
                    return 0;
                });

            // ·socket流关闭·回调函数
            nghttp2_session_callbacks_set_on_stream_close_callback(
                callbacks, 
                [](nghttp2_session *session, int32_t streamId,uint32_t errorCode, void *uptr)->int
                {
                    (void)errorCode;
                    (void)session;
                    auto self = coverFrom(uptr);
                    self->_data->m_stream.closeOne(streamId);
                    return 0;
                });

            // ·HTTP头帧·回调函数
            nghttp2_session_callbacks_set_on_header_callback(callbacks,
                [](nghttp2_session *session, nghttp2_frame const *frame, uint8_t const *name,
                   size_t nlen, uint8_t const *value, size_t vlen, uint8_t flags, void *uptr)->int
                {
                    // 在server端中，该函数用于指示和处理一个完整的HTTP请求的非数据部分。
                    // 这说明，一个HTTP请求被server端完整地接收后，数据的传输应该已经完成，
                    // 剩下的工作是处理之前在第一次接收到http头时没处理的工作。
                    // 注意，在整个请求的生命周期内内，该回调函数会被调用多次，且每次传递一个键值对。
                    auto self = coverFrom(uptr);
                    if (self->_data->m_onHeadKV)
                    {
                        return self->_data->m_onHeadKV( *frame, 
                            reinterpret_cast<const char*>(name),
                            reinterpret_cast<const char*>(value), flags);
                    }
                    return 0;
                });

            // ·HTTP开始头部帧处理·回调函数
            nghttp2_session_callbacks_set_on_begin_headers_callback(
                callbacks, 
                [](nghttp2_session *session, nghttp2_frame const *frame,void *uptr) ->int
                {
                    // 在server端中，该函数用于处理客户端请求的HTTP 头。
                    // 当客户端的发送的头被server接收到时，该函数会被立即调用。
                    // 这时候可以查看http头然后做一些准备，比如说创建数据接收缓冲区等。
                    auto self = coverFrom(uptr);
                    if(frame->hd.type != NGHTTP2_HEADERS 
                        ||( frame->headers.cat != NGHTTP2_HCAT_REQUEST
                        && frame->headers.cat != NGHTTP2_HCAT_PUSH_RESPONSE ))
                    {
                        return NGHTTP2_ERR_CALLBACK_FAILURE;
                    }

                    // 让用户处理一下头
                    // 由用户决定要不要继续下去
                    if (self->_data->m_onHeaderDone 
                        || 0 != self->_data->m_onHeaderDone(frame->headers))
                    {
                        return NGHTTP2_ERR_CALLBACK_FAILURE;
                    }

                    // 创建流并获取指向该流的指针
                    auto ptr = 
                        self->_data->m_stream
                        .createStreamData(frame->hd.stream_id);
                    
                    // 注册用户上下文指针
                    nghttp2_session_set_stream_user_data(
                        self->_data->m_session, 
                        frame->hd.stream_id,
                        self);
                    return 0;
                });

            // 创建会话
            nghttp2_session_server_new(&_data->m_session, callbacks, this);

            // 释放回调对象
            nghttp2_session_callbacks_del(callbacks);

        }

        static Http2rdSession * coverFrom(void * uptr)
        {
            return reinterpret_cast<Http2rdSession *>(uptr);
        }

        /// @brief 
        /// @return  0 == OK , -1 == disconnected , -2 == nghttp2 failed ,
        /// -3 == nghttp2 error , -4 == SSL error , -5 == SOCKET Error。
        /// 通常-4，可以进一步使用SSL的error接口查看进一步的情况，
        /// -3 也是同理。
        /// -1 和 -2 代表不可恢复的错误，这时候会主动断开连接。
        /// -5 表示Socket的read接口错误（返回-1），函数会处理非阻塞时
        /// (MSG_DONTWAIT或者O_NONBLOCK)接口无数据的情况，
        /// 因此用户需要处理其他错误，通过errno可以获取该错误。
        ssize_t recvAndStoreFromSocket()
        {
            std::string buffer;
            ssize_t size;

            size = getDataFromSSL(buffer);
            if (size < 0 )
            {
                return size; // SSL IO 似乎有错
            }
            else if(size ==0 )
            {
                size = getDataFromSocket(buffer);
                if (size < 0 )
                {
                    return size; // Socket似乎有错
                }
                else if(size ==0)
                {
                    return -1; // SSL 和Socket都不可用
                }
            }

            // 调用nghttp2接口接收实际数据
            size = buffer.size();
            size = nghttp2_session_mem_recv(
                        _data->m_session,
                        reinterpret_cast<uint8_t const *>(&buffer[0]),
                        size);
            if (size <0)
            {
                _data->m_stream.closeAll();
                return -2;
            }

            // 调用 nghttp2_session_mem_recv()
            // 可能会生成额外的待处理帧，因此在函数的末尾调用 session_send()。
            size = nghttp2_session_send(_data->m_session);
            if (size !=0)
            {
                return -3;
            }
            // ok
            return 0;
        }   

        /// @brief 
        /// @param buffer 
        /// @return 
        ssize_t getDataFromSSL(std::string & buffer)
        {
            if (_data->m_ssl)
            {
                auto resultPair = _data->m_ssl->read();
                if (resultPair.key()==0){
                    _data->m_stream.closeAll();
                    return -1;
                }
                else if(resultPair.key() < 0)
                {
                    return -4;
                }
                else {
                    buffer = std::move(resultPair.value());
                }
                return buffer.size();
            }
            return 0;
        }

        /// @brief 
        /// @param buffer 
        /// @return  
        ssize_t getDataFromSocket(std::string & buffer)
        {
            if(_data->m_sock)
            {
                std::string tmp;
                int ret = 0;
                tmp.resize(1024); // 1k buffer
                while(1)
                {
                    ret = _data->m_sock->recv(&tmp[0],tmp.size(),MSG_DONTWAIT);
                    if (ret >0)
                    {
                        buffer.append(tmp);
                    }else if (ret ==-1)
                    {
                        return 
                            (errno == EAGAIN || errno == EWOULDBLOCK)
                                ? 
                            buffer.size() : -5;
                    }
                    else if(ret == 0)
                    {
                        _data->m_stream.closeAll();
                        return -1;
                    }
                }
                return buffer.size();
            }
            return 0;
        }


    private:
        struct Data
        {
            nghttp2_session *m_session;
            Http2Stream m_stream;
            // 优先使用SSL IO；
            // 如果没有则使用无SSL的Socket IO。
            std::unique_ptr<aSSLSocketIO> m_ssl;
            std::shared_ptr<Socket> m_sock;
            std::function<int(nghttp2_headers const &)> m_onHeaderDone;
            std::function<int(nghttp2_frame const & ,std::string const & , std::string const & , uint8_t )> m_onHeadKV;
            std::function<int(nghttp2_frame const &)> m_onFrameDone;
        };
        std::unique_ptr<Data> _data;
    };

}
