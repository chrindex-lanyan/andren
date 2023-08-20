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

#include "sslstream.hh"

#include "../base/andren_base.hh"

#include "eventloop.hh"

namespace chrindex::andren::network
{
    class Http2rdFrame
    {
    public :

        Http2rdFrame();
        ~Http2rdFrame();


        std::string data;

    };


    class Http2rdStream
    {
    public :

        Http2rdStream();
        Http2rdStream(Http2rdStream&&) ;
        ~Http2rdStream();


        std::map<int, Http2rdFrame>frames; 
    };

    class Http2rdSession
    {
    public:
        Http2rdSession() 
        {
            
        }

        Http2rdSession(Http2rdSession &&_)
        {
            member = std::move(_.member);
        }
        ~Http2rdSession()
        {
            
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

        
    private :


    private:
        struct Data
        {
            nghttp2_session * session;
            SSLStream ssl;
            std::map<int, Http2rdStream> streams;
        };
        std::unique_ptr<Data> member;
    };

}


/*
    // 调用 nghttp2_session_mem_recv()
    // 可能会生成额外的待处理帧，因此在函数的末尾调用 session_send()。
    nghttp2_session_send

    // 调用nghttp2接口接收实际数据
    nghttp2_session_mem_recv

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