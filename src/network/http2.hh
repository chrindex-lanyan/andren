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
#include <sys/types.h>

#include "sslstream.hh"

#include "../base/andren_base.hh"

#include "eventloop.hh"

namespace chrindex::andren::network
{
    class Http2rdFrame
    {
    public :
        Http2rdFrame(){}
        Http2rdFrame(nghttp2_frame const *frame){}
        ~Http2rdFrame(){}


        std::string data;

    };


    class Http2rdStream
    {
    public :

        Http2rdStream(){}
        Http2rdStream(Http2rdStream && ) {}
        ~Http2rdStream(){}

        void operator=( Http2rdStream&& ){}

        int push_frame(Http2rdFrame && frame);


        std::map<int, Http2rdFrame>frames; 
        int id;
    };

    class Http2rdSession
    {
    public:

        // 1是server ， 2是client
        Http2rdSession( int endType  , SSLStream && sslstream) 
        {
            member = std::make_unique<_private>();
            member->endType = endType;
            member->ssl = std::move(sslstream);

            nghttp2_session_callbacks * cbs =0;

            nghttp2_session_callbacks_new(&cbs);

            regCallbacks(cbs);

            if (endType == 1)
            {
                nghttp2_session_server_new( &member->session, cbs, this);
            }
            else if (endType == 2)
            {
                nghttp2_session_client_new( &member->session, cbs, this);
            }

            nghttp2_session_callbacks_del(cbs);
        }

        Http2rdSession(Http2rdSession &&_)
        {
            member = std::move(_.member);
        }
        ~Http2rdSession()
        {
            if(member)
            {
                nghttp2_session_del(member->session);
            }
        }

        void operator=(Http2rdSession &&_)
        {
            this->~Http2rdSession();
            member = std::move(_.member);
        }

        bool valid() const 
        { 
            return member != nullptr;
        }

        // 异步地发送一个请求
        bool asendResponse();

    private :

        void regCallbacks( nghttp2_session_callbacks * cbs)
        {
            /**
             * 为了说清楚函数的被调用实机i，现假设一种情况：
             * 一个请求由10个帧组成，其中head部分占3个帧，数据部分占7个帧。
             * 则 ： 
             *   on_frame_recv_callback： 在每个帧接收后都会被调用一次，包括 HEADER 帧和 DATA 帧。在例子中，这个回调函数会在每个帧接收时都被调用共10次。
             *   on_header_callback： 在接收到完整的 HEADER 帧时会被调用。在例子中，这个回调函数会在接收完前3个 HEADER 帧后被调用1次。
             *   on_begin_headers_callback： 在每个头部块开始解析时被调用1次。在例子中，这个回调函数会在解析前3个头部块时各被调用1次。
             *   send_callback：当nghttp2想要发送响应时，会被调用至少1次。
             *   on_stream_close_callback：当[请求]--[响应]完成时会被回调1次。
             */

            
            // 实际`发送数据`的回调函数
            nghttp2_session_callbacks_set_send_callback(cbs, &Http2rdSession::realSend);

            // `收到并处理数据帧`（包括头部帧、数据帧、优先级帧、设置帧等）的回调函数
            nghttp2_session_callbacks_set_on_frame_recv_callback(cbs,&Http2rdSession::onFrameRecv);

            // ·socket流关闭·回调函数
            nghttp2_session_callbacks_set_on_stream_close_callback(cbs , &Http2rdSession::onStreamClose);

            // ·HTTP头帧·回调函数
            nghttp2_session_callbacks_set_on_header_callback(cbs , &Http2rdSession::onReqDone);

            // ·HTTP开始头部帧处理·回调函数
            nghttp2_session_callbacks_set_on_begin_headers_callback(cbs , &Http2rdSession::onHeader);

        }

        static ssize_t realSend(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
        {
            auto self = reinterpret_cast<Http2rdSession*>(user_data);
            bool bret = self->member->ssl.asend(std::string(reinterpret_cast<char const *>(data), length));

            return bret ? 0 : -1 ;
        }

        static int onFrameRecv(nghttp2_session *session,const nghttp2_frame *frame, void *user_data)
        {
            // 收到了帧，需要自己拿到stream然后push回去
            // 此时还不需要回调通知用户。
            // 当一个请求被完整地传送，其最后一个帧的flags 包含 END_STREAM 标志。

            // auto self = reinterpret_cast<Http2rdSession*>(user_data);
            auto stream_data = reinterpret_cast<Http2rdStream*>(nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));
            stream_data->push_frame(frame);

            

            return 0;
        }
        
        static int onStreamClose(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data)
        {
            auto self = reinterpret_cast<Http2rdSession*>(user_data);
            auto iter = self->member->streams.find(stream_id); 

            if(iter == self->member->streams.end()) [[unlikely]] // 几乎可以绝对保证不可为空
            {
                return -1;
            }
            // 回调通知用户
            // onStreamClosed(流, errcode, this)

            // 抹去stream
            self->member->streams.erase(iter);

            return 0;
        }

        static int onReqDone(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data)
        {
            /// 一个stream的一个请求的head部分完整地收到了。
            /// 然后回调通知用户，让用户处理这个请求。
            /// 用户可以决定处理，或者等待数据帧也传送完成再处理。

            auto self = reinterpret_cast<Http2rdSession*>(user_data);
            auto stream_data = reinterpret_cast<Http2rdStream*>(nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));

            std::string reqname( reinterpret_cast<char const *>(name), namelen  ) , reqvalue (reinterpret_cast<char const *>(value) ,valuelen );

            // 回调用户
            // onHeadDone(请求名，请求值，流， 标志， this)

            return 0;
        }
        
        static int onHeader(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
        {
            if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) 
            {
                return 0;
            }

            /// 新的头到来往往意味着新的流来了。
            /// 当然，一个请求的头假如有3个帧组成，那么函数会被回调3次。
            /// 实际上这里是预创建流了。

            auto self = reinterpret_cast<Http2rdSession*>(user_data);
            if (auto iter = self->member->streams.find(frame->hd.stream_id); 
                    iter == self->member->streams.end()) [[likely]] // 只看第1次
            {
                /// 新的流被创建了，应该通知应用层
                auto & stream = self->member->streams[frame->hd.stream_id] ;

                // 用不着给stream分配指向Http2Stream的user_data指针，因为能拿到Http2Session的指针。
                // 当然，如果分配的话，效率会更高，毕竟免去一次查表。
                nghttp2_session_set_stream_user_data(session, frame->hd.stream_id,
                                         &stream); 

                /// 用户回调
                // onNewHead(流，this);
            }
            
            return 0;
        }

        ssize_t realRecv(std::string && data)
        {
            return realRecv( data.c_str(), data.size() );
        }

        ssize_t realRecv(char const * data, size_t size)
        {
            ssize_t readlen ;

            readlen = nghttp2_session_mem_recv(member->session, reinterpret_cast<uint8_t const *>(data), size );
            if (readlen < 0) 
            {    
                return -1;
            }

            // 调用 nghttp2_session_mem_recv()
            // 可能会生成额外的待处理帧，因此在函数的末尾调用 nghttp2_session_send()。
            if (nghttp2_session_send(member->session) != 0) 
            {
                return -2;
            }
            return readlen;
        }

    private:
        struct _private
        {
            _private(){ session = 0; endType =0;  }

            nghttp2_session * session;
            SSLStream ssl;
            std::map<int, Http2rdStream> streams;
            int endType;
        };
        std::unique_ptr<_private> member;
    };

}


/*
    // 调用nghttp2接口接收实际数据
    nghttp2_session_mem_recv

    // 调用 nghttp2_session_mem_recv()
    // 可能会生成额外的待处理帧，因此在函数的末尾调用 session_send()。
    nghttp2_session_send

    nghttp2_session_callbacks_new();

    // 实际`发送数据`的回调函数
    // nghttp2_session_mem_send 和 nghttp2_send_callback 都是发数据的，
    // 一般来说时使用其中一个就够了。
    // 这里使用的是 nghttp2_send_callback 回调的方式发数据。
    // 也就是说让nghttp2自己决定什么时候发数据，而无需用户干涉。
    nghttp2_session_callbacks_set_send_callback();

    // `收到并处理数据帧`（包括头部帧、数据帧、优先级帧、设置帧等）的回调函数
    nghttp2_session_callbacks_set_on_frame_recv_callback();

    // ·socket流关闭·回调函数
    nghttp2_session_callbacks_set_on_stream_close_callback();

    // ·HTTP头帧·回调函数
    nghttp2_session_callbacks_set_on_header_callback();

    // ·HTTP开始头部帧处理·回调函数
    nghttp2_session_callbacks_set_on_begin_headers_callback();

    // 创建会话
    nghttp2_session_server_new();

    // 释放回调对象
    nghttp2_session_callbacks_del();

    // 删除会话
    nghttp2_session_del();

**/