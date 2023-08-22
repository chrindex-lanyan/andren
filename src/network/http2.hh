#pragma once

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

#include "eventloop.hh"

namespace chrindex::andren::network
{
    
    class Http2ndRequest
    {
    public :

        Http2ndRequest (){}
        Http2ndRequest (Http2ndRequest && ){}
        ~Http2ndRequest(){}

        Http2ndRequest(const std::string& method, const std::string& path,
            const std::map<std::string, std::string>& headers = {},
            const std::string& body = "")
            : method(method), path(path), headers(headers), body(body) {}

    private:

        std::string method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::string body;

    };


    class Http2ndResponse {
    public:
        Http2ndResponse(int status_code, const std::map<std::string, std::string>& headers = {},
                const std::string& body = "")
            : status_code(status_code), headers(headers), body(body) {}

    private:

        int status_code;
        std::map<std::string, std::string> headers;
        std::string body;
    };


    class Http2ndFrame
    {
    public :
        Http2ndFrame(){}

        Http2ndFrame(Http2ndFrame && ){}

        Http2ndFrame(nghttp2_frame const *frame)
        {
            switch (frame->hd.type) 
            {
            case NGHTTP2_DATA:{
                coverToDATA(frame);
                break;
            }
            case NGHTTP2_HEADERS:{
                coverToHEADERS(frame);
                break;
            }
            case NGHTTP2_PRIORITY:{
                coverToPRIORITY(frame);
                break;
            }
            case NGHTTP2_RST_STREAM:{
                coverToRST_STREAMr(frame);
                break;
            }
            case NGHTTP2_SETTINGS:{
                coverToSETTINGS(frame);
                break;
            }
            case NGHTTP2_PUSH_PROMISE:{
                coverToPUSH_PROMIS(frame);
                break;
            }
            case NGHTTP2_PING:{
                coverToPING(frame);
                break;
            }
            case NGHTTP2_GOAWAY:{
                coverToGOAWAY(frame);
                break;
            }
            case NGHTTP2_WINDOW_UPDATE:{
                coverToWINDOW_UPDATE(frame);
                break;
            }
            case NGHTTP2_CONTINUATION:{
                /* The CONTINUATION frame.  This frame type won't be passed to any
                    callbacks because the library processes this frame type and its
                    preceding HEADERS/PUSH_PROMISE as a single frame.
                */
                break;
            }
            case NGHTTP2_ALTSVC:{
                /*
                    The ALTSVC frame, which is defined in `RFC 7383
                */
                break;
            }
            case NGHTTP2_ORIGIN:{
                /*
                    The ORIGIN frame, which is defined by `RFC 8336
                */
                break;
            }
            default:
            {
                return ;
                break;
            }
            }
        }

        ~Http2ndFrame(){}

    private :

        void coverToDATA(nghttp2_frame const *frame )
        {
            // 数据会以其他回调的方式通知用户, 这里只有头信息。
            auto * pdf = getPointer<data_frame>();
            *pdf = frame->data;
        }

        void coverToHEADERS(nghttp2_frame const *frame)
        {
            auto * pheaders = getPointer<headers_frame>();

            pheaders->cat = frame->headers.cat;
            pheaders->hd = frame->headers.hd;
            pheaders->padlen = frame->headers.padlen;
            pheaders->pri_spec = frame->headers.pri_spec;
            for (size_t i = 0; i< frame->headers.nvlen; i++ )
            {
                pheaders->nvlist.push_back(frame->headers.nva[i]);
            }
        }

        void coverToPRIORITY(nghttp2_frame const *frame)
        {
            auto * p = getPointer<priority_frame>();

            *p = frame->priority;
        }

        void coverToRST_STREAMr(nghttp2_frame const *frame){}

        void coverToPING(nghttp2_frame const *frame){}

        void coverToPUSH_PROMIS(nghttp2_frame const *frame){}

        void coverToGOAWAY(nghttp2_frame const *frame){}

        void coverToWINDOW_UPDATE(nghttp2_frame const *frame){}

        void coverToSETTINGS(nghttp2_frame const *frame){}


        template<typename T>
        T * getPointer()
        {
            return reinterpret_cast<T *>(frame) ;
        }


    private :

        // 帧头部。保存了帧的头部信息
        // DATA 帧： 用于传输请求或响应的实际数据。它可以分为多个帧，通过 stream_id 标识数据所属的流。
        // HEADERS 帧： 用于传输请求或响应的头部信息。它也可以分为多个帧，通过 stream_id 标识头部信息所属的流。一个 HEADERS 帧可能会被后续的 CONTINUATION 帧补充。
        // PRIORITY 帧： 用于指定流的优先级，帮助服务器和客户端进行请求的调度和处理。
        // RST_STREAM 帧： 用于取消流或指示流的状态变化，例如取消请求或表示请求完成。
        // SETTINGS 帧： 用于传输连接级别和流级别的设置，例如流的最大并发数、窗口大小等。
        // PUSH_PROMISE 帧： 用于服务器向客户端推送资源的请求。其中包含了新流的标识符和推送资源的头部信息。
        // PING 帧： 用于检测连接的活跃性，可以由客户端或服务器发送。
        // GOAWAY 帧： 用于表示连接的结束，通常由服务器发送，告知客户端不再接受新的请求。
        // WINDOW_UPDATE 帧： 用于调整流级别和连接级别的流量窗口大小，用于流量控制。
        // 拓展帧。保存了HTTP2的拓展帧的相关数据。
        
        /**
        * The frame header, which is convenient to inspect frame header.
        */
        struct frame_hd
        {
            frame_hd(nghttp2_frame_hd const & hd){
                length = hd.length;
                stream_id = hd.stream_id;
                type = hd.type;
                flags = hd.flags;
                reserved = hd.reserved;
            }

            DEFAULT_CONSTRUCTION(frame_hd);
            DEFAULT_COPY_CONSTRUCTION(frame_hd);
            DEFAULT_COPY_OPERATOR(frame_hd);

            /**
            * The length field of this frame, excluding frame header.
            */
            size_t length;

            /**
            * The stream identifier (aka, stream ID)
            */
            int32_t stream_id;

            /**
            * The type of this frame.  See `nghttp2_frame_type`.
            */
            uint8_t type;

            /**
            * The flags.
            */
            uint8_t flags;
            
            /**
            * Reserved bit in frame header.  Currently, this is always set to 0
            * and application should not expect something useful in here.
            */
            uint8_t reserved;
        } ;

        /**
        * The DATA frame.
        */
        struct data_frame
        {
            data_frame(nghttp2_data const & d)
            {
                hd = d.hd;
                padlen = d.padlen;
            }

            DEFAULT_CONSTRUCTION(data_frame);
            DEFAULT_COPY_CONSTRUCTION(data_frame);
            DEFAULT_COPY_OPERATOR(data_frame);

            frame_hd hd;
            /**
            * The length of the padding in this frame.  This includes PAD_HIGH
            * and PAD_LOW.
            */
            size_t padlen;
        };

        struct priority_spec
        {
            priority_spec(nghttp2_priority_spec const& spec){
                stream_id = spec.stream_id;
                weight = spec.weight;
                exclusive = spec.exclusive;
            }

            DEFAULT_CONSTRUCTION(priority_spec);
            DEFAULT_COPY_CONSTRUCTION(priority_spec);
            DEFAULT_COPY_OPERATOR(priority_spec);

            /**
            * The stream ID of the stream to depend on.  Specifying 0 makes
            * stream not depend any other stream.
            */
            int32_t stream_id;

            /**
            * The weight of this dependency.
            */
            int32_t weight;

            /**
            * nonzero means exclusive dependency
            */
            uint8_t exclusive;
        };

        /**
        * The name/value pairs.
        */
        struct name_value
        {
            name_value(void const * n , size_t ns ,void const * v, size_t vs , uint8_t _flags)
            {
                name = std::string(reinterpret_cast<char const *>(n) , ns);
                value = std::string(reinterpret_cast<char const *>(v), vs);
                flags = _flags;
            }
            name_value(nghttp2_nv const & nv)
            {
                name = std::string(reinterpret_cast<char const *>(nv.name) , nv.namelen);
                value = std::string(reinterpret_cast<char const *>(nv.value) , nv.valuelen);
                flags = nv.flags;
            }

            DEFAULT_CONSTRUCTION(name_value);
            DEFAULT_COPY_CONSTRUCTION(name_value);
            DEFAULT_COPY_OPERATOR(name_value);

            /**
            * The |name| byte string.  If this struct is presented from library
            * (e.g., :type:`nghttp2_on_frame_recv_callback`), |name| is
            * guaranteed to be NULL-terminated.  For some callbacks
            * (:type:`nghttp2_before_frame_send_callback`,
            * :type:`nghttp2_on_frame_send_callback`, and
            * :type:`nghttp2_on_frame_not_send_callback`), it may not be
            * NULL-terminated if header field is passed from application with
            * the flag :enum:`nghttp2_nv_flag.NGHTTP2_NV_FLAG_NO_COPY_NAME`).
            * When application is constructing this struct, |name| is not
            * required to be NULL-terminated.
            */
            std::string name;

            /**
            * The |value| byte string.  If this struct is presented from
            * library (e.g., :type:`nghttp2_on_frame_recv_callback`), |value|
            * is guaranteed to be NULL-terminated.  For some callbacks
            * (:type:`nghttp2_before_frame_send_callback`,
            * :type:`nghttp2_on_frame_send_callback`, and
            * :type:`nghttp2_on_frame_not_send_callback`), it may not be
            * NULL-terminated if header field is passed from application with
            * the flag :enum:`nghttp2_nv_flag.NGHTTP2_NV_FLAG_NO_COPY_VALUE`).
            * When application is constructing this struct, |value| is not
            * required to be NULL-terminated.
            */
            std::string value;

            /**
            * Bitwise OR of one or more of :type:`nghttp2_nv_flag`.
            */
            uint8_t flags;
        } ;

        /**
        * The HEADERS frame.
        */
        struct headers_frame
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The length of the padding in this frame.  This includes PAD_HIGH
            * and PAD_LOW.
            */
            size_t padlen;

            /**
            * The priority specification
            */
            struct priority_spec pri_spec;

            /**
            * The number of name/value pairs in |nva|.
            */
            std::vector<name_value> nvlist;

            /**
            * The category(ENUM) of this HEADERS frame.
            */
            nghttp2_headers_category cat; 

        } ;

        /**
        * The PRIORITY frame.
        */
        struct priority_frame
        {
            priority_frame(nghttp2_priority const & p)
            {
                hd = p.hd;
                pri_spec = p.pri_spec;
            }

            DEFAULT_CONSTRUCTION(priority_frame);
            DEFAULT_COPY_CONSTRUCTION(priority_frame);
            DEFAULT_COPY_OPERATOR(priority_frame);

            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The priority specification.
            */
            struct priority_spec  pri_spec;

        };

        struct rst_stream
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The error code.  See :type:`nghttp2_error_code`.
            */
            uint32_t error_code;
        }  ;

        /**
        * The pointer to the array of SETTINGS ID/Value pair.
        */
        struct settings_entry
        {
            /**
            * The SETTINGS ID.  See :type:`nghttp2_settings_id`.
            */
            int32_t settings_id;

            /**
            * The value of this entry.
            */
            uint32_t value;
        } ;

        /**
        * The SETTINGS frame.
        */
        struct settings_frame
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /** ID/Value Pairs **/
            std::vector<settings_entry> ivlist;
        } ;

        /**
        * The PUSH_PROMISE frame.
        */
        struct push_promise
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The length of the padding in this frame.  This includes PAD_HIGH
            * and PAD_LOW.
            */
            size_t padlen;

            // /**
            // * The name/value pairs.
            // */
            std::vector<name_value> nvlist;

            /**
            * The promised stream ID
            */
            int32_t promised_stream_id;

            /**
            * Reserved bit.  Currently this is always set to 0 and application
            * should not expect something useful in here.
            */
            uint8_t reserved;
        };

        /**
        * The PING frame.
        */
        struct ping_frame
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The opaque data
            */
            uint8_t opaque_data[8];
        } ;


        /**
        * The GOAWAY frame.
        */
        struct goaway_frame
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The last stream stream ID.
            */
            int32_t last_stream_id;

            /**
            * The error code.  See :type:`nghttp2_error_code`.
            */
            uint32_t error_code;

            // /**
            // * The additional debug data
            // */
            // uint8_t *opaque_data;

            // /**
            // * The length of |opaque_data| member.
            // */
            // size_t opaque_data_len;
            
            std::string opaque_data;

            /**
            * Reserved bit.  Currently this is always set to 0 and application
            * should not expect something useful in here.
            */
            uint8_t reserved;
        } ;

        /**
        * The WINDOW_UPDATE frame.
        */
        struct window_update_frame
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The window size increment.
            */
            int32_t window_size_increment;

            /**
            * Reserved bit.  Currently this is always set to 0 and application
            * should not expect something useful in here.
            */
            uint8_t reserved;
        } ;

         /**
        * The extension frame.
        */
        struct extension_frame
        {
            /**
            * The frame header.
            */
            frame_hd hd;

            /**
            * The pointer to extension payload.  The exact pointer type is
            * determined by hd.type.
            *
            * Currently, no extension is supported.  This is a place holder for
            * the future extensions.
            */
            void *payload;
        } ;
        
        // using frame_type = VARIANT_TYPE(frame_hd, data_frame, headers_frame , priority_frame , rst_stream \
        //     , settings_frame, push_promise , ping_frame , goaway_frame, window_update_frame , extension_frame) ;

        union _Frame_union{ frame_hd hd;  data_frame data;  headers_frame headers; priority_frame priority; rst_stream rst;
                settings_frame settings; push_promise ppromise; ping_frame ping;  goaway_frame goaway; window_update_frame wupdate; extension_frame ext; };

        using frame_type = char[sizeof(_Frame_union)];

        frame_type frame = {0};
    };

    class Http2ndStream
    {
    public :

        Http2ndStream(){}
        Http2ndStream(Http2ndStream && ) {}
        ~Http2ndStream(){}

        void operator=( Http2ndStream&& ){}

        int push_frame(Http2ndFrame && frame)
        {
            frames.push_back(std::move(frame));
            return 0;
        }

        int push_data(std::string && data);
        
        Http2ndRequest tryFetchRequest();

        Http2ndResponse tryFetchResponse();


        std::vector<Http2ndFrame>frames; 
        int id;
        std::string reqname;
        std::string reqvalue;
        uint8_t onHeadFlags;
        std::string data;
    };

    class Http2ndSession
    {
    public:

        /// @brief [请求]完整接收通知。该函数对于[响应]也适用。
        /// onReqRecvDone(流,this);
        /// 当一个[请求]或[响应]被完整地传送，其最后一个帧的flags 包含 NGHTTP2_FLAG_END_STREAM 标志，
        /// 这时就需要回调此函数以通知用户：[请求]或[响应]被完整地接收了。
        /// 请注意，因为Head和Push_Promise帧可能会被分割成更小的CONTINUATION 帧。
        /// 因此在整个Head或者Push_Promise帧完成前不会被回调，也就是说CONTINUATION 帧
        /// 对此回调函数是透明的。
        using OnReqRecvDone = std::function<int(Http2ndStream * , Http2ndSession *)>;

        /// @brief 流被关闭回调函数。
        /// onStreamClosed(流, Nghttp2的错误码, this);
        /// 通知用户这个流被关闭了。在该回调结束后，流结构的内存会被释放。
        using OnStreamClosed = std::function<int(Http2ndStream * , uint32_t errcode, Http2ndSession *)>;

        /// @brief 头完整接收回调函数。
        /// onHeadDone(流， this);
        /// 一个stream的请求的head部分完整地收到了。
        /// 我在此处只保存reqname和reqvalue，因为nghttp2帮我处理HPACK的情况了。
        /// 我在此处进行回调以通知用户。
        using OnHeadDone = std::function<int(Http2ndStream *, Http2ndSession *)>;

        /// @brief 流创建通知回调函数。
        /// onNewHead(流，this);
        /// 新的头到来往往意味着新的流来了。
        /// 当然，一个请求的头假如有3个帧组成，那么内部的某个函数会被回调3次，
        /// 但OnNewHead只会在第一次被回调，实际上是通知用户一个新的流被创建了。
        using OnNewHead = std::function<int(Http2ndStream * , Http2ndSession *)>;

        /**
        * @brief 数据块接收回调函数。
        * 虽然每个帧都会回调nghttp2OnFrameRecv，但nghttp2_frame 结构并不包含数据部分，因此需要使用
        * 此回调函数接收数据块。用户可以在此回调提前处理数据，这特别适用于流媒体。
        * 需要注意，为了减少不必要的大数据块拷贝，数据不会被主动push_data到stream里，如果有需要用户需在回调里自己push_data。
        */
        using OnDataChunkRecv = std::function<int( Http2ndStream * , uint8_t flags, std::string && data , Http2ndSession *)>;


        // 1是server ， 2是client
        Http2ndSession( int endType  , SSLStream && sslstream) ;

        Http2ndSession(Http2ndSession &&_);

        ~Http2ndSession();

        void operator=(Http2ndSession &&_);

        // 是否可用
        bool valid() const ;

        // 异步地发送一个请求
        bool asendResponse();

    private :

        void regCallbacks( nghttp2_session_callbacks * cbs);

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
            SSLStream ssl;
            std::map<int, Http2ndStream> streams;
            int endType;

            // callbacks group
            OnReqRecvDone onReqRecvDone;
            OnStreamClosed onStreamClosed;
            OnHeadDone onHeadDone;
            OnNewHead onNewHead;
            OnDataChunkRecv onDataChunkRecv;
            
        };
        std::unique_ptr<_private> member;
    };

}
