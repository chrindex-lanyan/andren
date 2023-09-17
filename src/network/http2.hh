#pragma once

#include <cstring>
#include <functional>
#include <sys/socket.h>
#include <fcntl.h>


#include <nghttp2/nghttp2.h>
#include <nghttp2/nghttp2ver.h>

#include <string>
#include <stdint.h>
#include <memory>
#include <map>
#include <optional>
#include <variant>
#include <stdexcept>
#include <any>
#include <sys/types.h>
#include <utility>

#include "sslstream.hh"

#include "../base/andren_base.hh"

#include "task_distributor.hh"

#include "http2frame.hh"

namespace chrindex::andren::network
{
    class Http2ndRequest
    {
    public :
        Http2ndRequest(std::map<std::string, std::string> const & headers)
            :  headers(headers){}

        DEFAULT_MOVE_OPERATOR(Http2ndRequest);
        DEFAULT_MOVE_CONSTRUCTION(Http2ndRequest);
        DEFAULT_CONSTRUCTION(Http2ndRequest);
        DEFAULT_DECONSTRUCTION(Http2ndRequest);
        DEFAULT_COPY_CONSTRUCTION(Http2ndRequest);
        DEFAULT_COPY_OPERATOR(Http2ndRequest);

    public:
        std::map<std::string, std::string> headers;
    };


    class Http2ndResponse {
    public:
        Http2ndResponse(std::map<std::string, std::string> const & headers)
            :  headers(headers){}

        DEFAULT_MOVE_OPERATOR(Http2ndResponse);
        DEFAULT_MOVE_CONSTRUCTION(Http2ndResponse);
        DEFAULT_CONSTRUCTION(Http2ndResponse);
        DEFAULT_DECONSTRUCTION(Http2ndResponse);
        DEFAULT_COPY_CONSTRUCTION(Http2ndResponse);
        DEFAULT_COPY_OPERATOR(Http2ndResponse);

    public:
        std::map<std::string, std::string> headers;
    };


    class Http2ndStream
    {
    public :

        Http2ndStream(){ id = 0; recvFrameFlags= false; }
        ~Http2ndStream(){}

        DEFAULT_COPY_CONSTRUCTION(Http2ndStream);
        DEFAULT_COPY_OPERATOR(Http2ndStream);
        DEFAULT_MOVE_CONSTRUCTION(Http2ndStream);
        DEFAULT_MOVE_OPERATOR(Http2ndStream);

        int push_frame(Http2ndFrame && frame)
        {
            if (recvFrameFlags){
                frames.push_back(std::forward<Http2ndFrame>(frame));
            }
            
            return 0;
        }

        int push_data(std::string && data)
        { 
            if (std::holds_alternative<Http2ndRequest>(derive_data)){
                std::get<Http2ndRequest>(derive_data).headers["body"].append(std::forward<std::string>(data));
            }
            else if(std::holds_alternative<Http2ndResponse>(derive_data)){
                std::get<Http2ndResponse>(derive_data).headers["body"].append(std::forward<std::string>(data));
            }
            else {
                return -1;
            }
            return 0;
        }

        int push_header(std::string && name, std::string &&value)
        {
            if (std::holds_alternative<Http2ndRequest>(derive_data)){
                std::get<Http2ndRequest>(derive_data).headers[std::forward<std::string>(name)] = std::forward<std::string>(value);
            }
            else if(std::holds_alternative<Http2ndResponse>(derive_data)){
                std::get<Http2ndResponse>(derive_data).headers[std::forward<std::string>(name)] = std::forward<std::string>(value);
            }
            else {
                return -1;
            }
            return 0;
        }

        Http2ndRequest * referent_request()
        {
            if (std::holds_alternative<Http2ndRequest>(derive_data)){
                return &std::get<Http2ndRequest>(derive_data);
            }
            return nullptr;
        }
        
        Http2ndResponse * referent_response()
        {
            if(std::holds_alternative<Http2ndResponse>(derive_data)){
                return &std::get<Http2ndResponse>(derive_data);
            }
            return nullptr;
        }

        int initAs(Http2ndRequest && real_struct)
        {
            derive_data = std::forward<Http2ndRequest>(real_struct); 
            return 0;
        }

        int initAs(Http2ndResponse && real_struct)
        {
            derive_data = std::forward<Http2ndResponse>(real_struct);
            return 0;
        }

        void enableFrameSave(bool enabledOrNot)
        {
            recvFrameFlags = enabledOrNot;
        }

        void clearFrameHistory()
        {
            frames.clear();
        }

        int id;
        VARIANT_TYPE(Http2ndRequest, Http2ndResponse) derive_data;
        std::vector<Http2ndFrame> frames; 
        bool recvFrameFlags;
    };

    class Http2ndSession
    {
    public:

        /// @brief [请求]完整接收通知。该函数对于[响应]也适用。
        /// onReqRecvDone(流,this);
        /// 当一个[请求]或[响应]被完整地传送，其最后一个帧的flags 包含 NGHTTP2_FLAG_END_STREAM 标志，
        /// 这时就需要回调此函数以通知用户：[请求]或[响应]被完整地接收了。
        using OnReqRecvDone = std::function<int(Http2ndStream * , Http2ndSession *)>;

        /// @brief 流被关闭回调函数。
        /// onStreamClosed(流, Nghttp2的错误码, this);
        /// 通知用户这个流被关闭了。在该回调结束后，流结构的内存会被释放。
        using OnStreamClosed = std::function<int(Http2ndStream * , uint32_t errcode, Http2ndSession *)>;

        /// @brief 头完整接收回调函数。
        /// onHeadDone(流， this);
        /// 每次回调提供header的键值对，包括可以获得request_path（key =":path"）,
        /// method（key=":method"），status_code（key=":status"）等。
        /// 一次回调只会提供一个键值对，因此该函数会被回调多次。
        using OnHeadDone = std::function<int(Http2ndStream *, Http2ndSession *)>;

        /** @brief 流创建通知回调函数。
        * onNewHead(流，this);
        * 新的头到来往往意味着新的流来了。
        * 这个函数会在一个流的请求或响应被调用一次。
        * 可以通过这个回调获取一些必要信息，比如：request、response或者push_promise。
        */ 
        using OnNewHead = std::function<int(Http2ndStream * , Http2ndSession *)>;

        /**
        * @brief 数据块接收回调函数。
        * 虽然每个帧都会回调nghttp2OnFrameRecv，但nghttp2_frame 结构并不包含数据部分，因此需要使用
        * 此回调函数接收数据块。用户可以在此回调提前处理数据，这特别适用于流媒体。
        * 需要注意，为了减少不必要的大数据块拷贝，数据不会被主动push_data到stream里，如果有需要用户可在回调里自己push_data。
        */
        using OnDataChunkRecv = std::function<int( Http2ndStream * , uint8_t flags, std::string && data , Http2ndSession *)>;

        /**
         * @brief 当推送承诺帧到达。
         * 服务器可以向客户端主动推送一个资源承诺帧。
         * 这个帧提示客户端可能会在未来使用到的资源，
         * 包含了这些资源的请求方式等信息。
         */
        using OnPushPromiseFrame = std::function<int(Http2ndStream * , Http2ndSession *  , Http2ndFrame::push_promise const &)>;

        /**
         * @brief 关闭会话通知。
         * 当用于通信的socket fd在read时返回0，则触发此回调。
         */
        using OnSessionClose = std::function<void()>;


        struct CBGroup
        {
            OnReqRecvDone onReqRecvDone;
            OnStreamClosed onStreamClosed;
            OnHeadDone onHeadDone;
            OnNewHead onNewHead;
            OnDataChunkRecv onDataChunkRecv;
            OnPushPromiseFrame onPushPromiseFrame;
            OnSessionClose onSessionClose;
        };


        void setOnCallback(CBGroup && cbg);


        // 1是server ， 2是client
        Http2ndSession( int endType  , SSLStream && sslstream) ;

        Http2ndSession(Http2ndSession &&_);

        ~Http2ndSession();

        void operator=(Http2ndSession &&_);

        // 是否可用
        bool valid() const ;

        // 异步地发送一个响应
        bool asendResponse(Http2ndResponse const & );

        // 异步地发送一个请求
        bool asendRequeste(Http2ndRequest const & );

        Http2ndStream * findStreamById(int stream_id);

        bool eraseStreamById(int stream_id);

        nghttp2_session * sessionReference() const;

        SSLStream * streamReference() const;

        bool tryHandShakeAndInit();

    private :

        void regCallbacks( nghttp2_session_callbacks * cbs ,int endType);

        static ssize_t nghttp2RealSend(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);

        static int nghttp2OnFrameRecv(nghttp2_session *session,const nghttp2_frame *frame, void *user_data);
        
        static int nghttp2OnStreamClose(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data);

        static int nghttp2OnHeadDone(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data);
        
        static int nghttp2OnHeader(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);

        static int nghttp2OnDataChunkRecv(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data);

        ssize_t nghttp2RealRecv(std::string && data);

        ssize_t nghttp2RealRecv(char const * data, size_t size);

    private:
        struct _private
        {
            _private(){ session = 0; endType =0;  }

            nghttp2_session * session;
            std::shared_ptr<SSLStream> ssl;
            std::map<int, Http2ndStream> streams;
            int endType;

            // callbacks group
            OnReqRecvDone onReqRecvDone;
            OnStreamClosed onStreamClosed;
            OnHeadDone onHeadDone;
            OnNewHead onNewHead;
            OnDataChunkRecv onDataChunkRecv;
            OnPushPromiseFrame onPushPromiseFrame;
            OnSessionClose onSessionClose;
        };
        std::unique_ptr<_private> member;
    };

    /// @brief 该函数直接调用nghttp2_select_next_protocol。
    extern int proxy_nghttp2_select_next_protocol(
        unsigned char **out,unsigned char *outlen,
        const unsigned char *in, unsigned int inlen);

}
