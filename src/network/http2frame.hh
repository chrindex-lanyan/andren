#pragma once

#include <nghttp2/nghttp2.h>
#include <nghttp2/nghttp2ver.h>

#include "../base/andren_base.hh"


namespace chrindex::andren::network{

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
                coverToRST_STREAM(frame);
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
                break;
            }
            }
        }

        ~Http2ndFrame(){}

    private :

        void coverToDATA(nghttp2_frame const *frame )
        {
            // 数据会以其他回调的方式通知用户, 这里只有头信息。
            coverAndSave<data_frame>( frame->data );
        }

        void coverToHEADERS(nghttp2_frame const *frame)
        {
            coverAndSave<headers_frame>( frame->headers );
        }

        void coverToPRIORITY(nghttp2_frame const *frame)
        {
            coverAndSave<priority_frame>( frame->priority );
        }

        void coverToRST_STREAM(nghttp2_frame const *frame)
        {
            coverAndSave<rst_stream>( frame->rst_stream );
        }

        void coverToPING(nghttp2_frame const *frame)
        {
            coverAndSave<ping_frame>( frame->ping );
        }

        void coverToPUSH_PROMIS(nghttp2_frame const *frame)
        {
            coverAndSave<push_promise>( frame->push_promise );
        }

        void coverToGOAWAY(nghttp2_frame const *frame)
        {
            coverAndSave<goaway_frame>( frame->goaway );
        }

        void coverToWINDOW_UPDATE(nghttp2_frame const *frame)
        {
            coverAndSave<window_update_frame>( frame->window_update );
        }

        void coverToSETTINGS(nghttp2_frame const *frame)
        {
            coverAndSave<settings_frame>( frame->settings );
        }

        template<typename T>
        T * getPointer()
        {
            return reinterpret_cast<T *>(frame) ;
        }

        template <typename TargetType , typename SrcType>
        void coverAndSave(SrcType & s)
        {
            auto * p = getPointer<TargetType>();
            *p = s;
        }


    public :

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
            headers_frame(nghttp2_headers const & h)
            {
                cat = h.cat;
                hd = h.hd;
                padlen = h.padlen;
                pri_spec = h.pri_spec;
                for (size_t i = 0; i< h.nvlen; i++ )
                {
                    nvlist.push_back(h.nva[i]);
                }
            }

            DEFAULT_CONSTRUCTION(headers_frame);
            DEFAULT_COPY_CONSTRUCTION(headers_frame);
            DEFAULT_COPY_OPERATOR(headers_frame);

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
            rst_stream(nghttp2_rst_stream const & rst)
            {
                hd = rst.hd;
                error_code = rst.error_code;
            }

            DEFAULT_CONSTRUCTION(rst_stream);
            DEFAULT_COPY_CONSTRUCTION(rst_stream);
            DEFAULT_COPY_OPERATOR(rst_stream);

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
            settings_entry(nghttp2_settings_entry const & se)
            {
                settings_id = se.settings_id;
                value = se.value;
            }

            DEFAULT_CONSTRUCTION(settings_entry);
            DEFAULT_COPY_CONSTRUCTION(settings_entry);
            DEFAULT_COPY_OPERATOR(settings_entry);

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
            settings_frame(nghttp2_settings const & s)
            {
                hd = s.hd;
                for (size_t i = 0 ; i< s.niv ; i++)
                {
                    ivlist.push_back(s.iv[i]);
                }
            }

            DEFAULT_CONSTRUCTION(settings_frame);
            DEFAULT_COPY_CONSTRUCTION(settings_frame);
            DEFAULT_COPY_OPERATOR(settings_frame);

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
            push_promise(nghttp2_push_promise const & pp)
            {
                hd = pp.hd;
                padlen = pp.padlen;
                promised_stream_id = pp.promised_stream_id;
                reserved = pp.reserved;

                for (size_t i =0; i<pp.nvlen ; i++)
                {
                    nvlist.push_back( pp.nva[i]);
                }
            }

            DEFAULT_CONSTRUCTION(push_promise);
            DEFAULT_COPY_CONSTRUCTION(push_promise);
            DEFAULT_COPY_OPERATOR(push_promise);

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
            ping_frame(nghttp2_ping const & p)
            {
                hd = p.hd;
                ::memcpy(opaque_data,  p.opaque_data, sizeof(opaque_data)) ;
            }

            DEFAULT_CONSTRUCTION(ping_frame);
            DEFAULT_COPY_CONSTRUCTION(ping_frame);
            DEFAULT_COPY_OPERATOR(ping_frame);

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
            goaway_frame(nghttp2_goaway const & p)
            {
                hd = p.hd;
                last_stream_id = p.last_stream_id;
                error_code = p.error_code;
                opaque_data = { reinterpret_cast<char const *>(p.opaque_data), p.opaque_data_len  };
                reserved = p.reserved;
            }

            DEFAULT_CONSTRUCTION(goaway_frame);
            DEFAULT_COPY_CONSTRUCTION(goaway_frame);
            DEFAULT_COPY_OPERATOR(goaway_frame);

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
            
            /**
             * The additional debug data
             */
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
            window_update_frame(nghttp2_window_update const & wu)
            {
                hd = wu.hd;
                window_size_increment = wu.window_size_increment;
                reserved = wu.reserved;
            }

            DEFAULT_CONSTRUCTION(window_update_frame);
            DEFAULT_COPY_CONSTRUCTION(window_update_frame);
            DEFAULT_COPY_OPERATOR(window_update_frame);

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
            extension_frame(nghttp2_extension const & ext)
            {
                hd = ext.hd;
                payload = ext.payload;
            }

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

        union _Frame_union{ frame_hd hd;  data_frame data;  headers_frame headers; priority_frame priority; rst_stream rst;
                settings_frame settings; push_promise ppromise; ping_frame ping;  goaway_frame goaway; window_update_frame wupdate; extension_frame ext; };

        using frame_utype = _Frame_union;
        using frame_type = char[sizeof(frame_utype)];

        frame_type frame = {0};
    };

    }