
#include "http2.hh"
#include <nghttp2/nghttp2.h>


namespace chrindex::andren::network
{

    Http2ndSession::Http2ndSession( int endType  , SSLStream && sslstream) 
    {
        member = std::make_unique<_private>();
        member->endType = endType;
        member->ssl = std::move(sslstream);

        member->ssl.setOnClose([this]()
        {
            nghttp2RealRecv(0,0);
            member->onSessionClose();
        });

        member->ssl.setOnRead([this](ssize_t ret , std::string && data)
        {
            if (ret >0)
            {
                nghttp2RealRecv(std::move(data));
            }
        });
        member->ssl.setOnWrite([](auto ...){});

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

    Http2ndSession::Http2ndSession(Http2ndSession &&_)
    {
        member = std::move(_.member);
    }
    Http2ndSession::~Http2ndSession()
    {
        if(member)
        {
            nghttp2_session_del(member->session);
        }
    }

    void Http2ndSession::operator=(Http2ndSession &&_)
    {
        this->~Http2ndSession();
        member = std::move(_.member);
    }

    void Http2ndSession::setOnCallback(CBGroup && cbg)
    {
        member->onReqRecvDone = std::move(cbg.onReqRecvDone);
        member->onStreamClosed = std::move(cbg.onStreamClosed);
        member->onHeadDone = std::move(cbg.onHeadDone);
        member->onNewHead = std::move(cbg.onNewHead);
        member->onDataChunkRecv = std::move(cbg.onDataChunkRecv);
        member->onPushPromiseFrame = std::move(cbg.onPushPromiseFrame);
        member->onSessionClose = std::move(cbg.onSessionClose);
    }

    bool Http2ndSession::tryHandShakeAndInit()
    {
        if(1 != member->ssl.reference_sslio()->handShake())
        {
            return false;
        }
        return member->ssl.startListenReadEvent();
    }

    bool Http2ndSession::valid() const 
    { 
        return member != nullptr;
    }

    bool Http2ndSession::asendResponse(Http2ndResponse const & )
    {
        return true;
    }

    bool Http2ndSession::asendRequeste(Http2ndRequest const & )
    {
        return true;
    }

    nghttp2_session * Http2ndSession::sessionReference() const
    {
        return member->session;
    }

    SSLStream * Http2ndSession::streamReference()const
    {
        return &member->ssl;
    }

    void Http2ndSession::regCallbacks( nghttp2_session_callbacks * cbs)
    {
        // 实际`发送数据`的回调函数
        nghttp2_session_callbacks_set_send_callback(cbs, &Http2ndSession::nghttp2RealSend);

        // `帧收到`（包括头部帧、数据帧、优先级帧、设置帧等）的回调函数
        // 这个回调还会判断帧有没有收完。
        nghttp2_session_callbacks_set_on_frame_recv_callback(cbs,&Http2ndSession::nghttp2OnFrameRecv);

        // ·socket流关闭·回调函数。
        nghttp2_session_callbacks_set_on_stream_close_callback(cbs , &Http2ndSession::nghttp2OnStreamClose);

        // ·HTTP头帧·回调函数.header的name-value在这。
        nghttp2_session_callbacks_set_on_header_callback(cbs , &Http2ndSession::nghttp2OnHeadDone);

        // ·HTTP开始头部帧处理·回调函数.这个函数可以通知流的创建、即将到来的类型（request、response、push_promise）。
        nghttp2_session_callbacks_set_on_begin_headers_callback(cbs , &Http2ndSession::nghttp2OnHeader);

        // HTTP数据块接收回调。这个函数会在接收data块完成时回调。
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(cbs, &Http2ndSession::nghttp2OnDataChunkRecv);

    }

    ssize_t Http2ndSession::nghttp2RealSend(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        auto self = reinterpret_cast<Http2ndSession*>(user_data);
        bool bret = self->member->ssl.asend(std::string(reinterpret_cast<char const *>(data), length));

        return bret ? 0 : -1 ;
    }

    int Http2ndSession::nghttp2OnFrameRecv(nghttp2_session *session,const nghttp2_frame *frame, void *user_data)
    {
        // 收到了帧，需要自己拿到stream然后push回去
        // 此时还不需要回调通知用户。
        // 当一个请求被完整地传送，其最后一个帧的flags 包含 NGHTTP2_FLAG_END_STREAM 标志，
        // 这时就需要回调用户了。
        // 请注意，因为Head和Push_Promise帧可能会被分割成更小的CONTINUATION 帧。
        // 因此在整个Head或者Push_Promise帧完成前不会被回调，也就是说CONTINUATION 帧
        // 对此回调函数是透明的。

        int ret =0 ;
        auto self = reinterpret_cast<Http2ndSession*>(user_data);

        // 我翻阅了源码，该函数确实会在其他帧处理回调函数之后才被回调，这意味着流在其他的回调函数已被创建。
        // 且流的用户数据指针不为空。
        // 为了避免不必要的查找，在流被创建时我已将流指针放到用户数据指针里。
        auto stream_data = reinterpret_cast<Http2ndStream*>(nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));
        stream_data->push_frame(frame);

        if(frame->data.hd.flags & NGHTTP2_FLAG_END_HEADERS) // 头完整地收到了。
        {
            // 在这个回调里，用户可以开始处理完整的header。
            ret = self->member->onHeadDone(stream_data, self);
        }

        if(ret !=0)
        {
            return ret;
        }

        if (frame->data.hd.flags & NGHTTP2_FLAG_END_STREAM)
        {
            // 已是最后一帧, 回调用户
            ret = self->member->onReqRecvDone(stream_data,self);
        }

        return ret;
    }
    
    int Http2ndSession::nghttp2OnStreamClose(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data)
    {
        auto self = reinterpret_cast<Http2ndSession*>(user_data);
        auto iter = self->member->streams.find(stream_id); 

        if(iter == self->member->streams.end()) [[unlikely]] // 几乎可以绝对保证不可为空
        {
            return 0;
        }
        // 回调通知用户
        int ret = self->member->onStreamClosed(&iter->second , error_code , self );

        // 抹去stream
        self->member->streams.erase(iter);

        return ret;
    }

    int Http2ndSession::nghttp2OnHeadDone(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data)
    {
        //auto self = reinterpret_cast<Http2ndSession*>(user_data);
        auto stream_data = reinterpret_cast<Http2ndStream*>(nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));

        std::string _name( reinterpret_cast<char const *>(name), namelen) , _value (reinterpret_cast<char const *>(value) ,valuelen);

        //  保存到流
        stream_data->push_header(std::move(_name), std::move(_value)) ;
        //stream_data->onHeadFlags = flags; 

        return 0;
    }
    
    int Http2ndSession::nghttp2OnHeader(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        if (frame->hd.type != NGHTTP2_HEADERS 
            || frame->headers.cat != NGHTTP2_HCAT_REQUEST
            || frame->headers.cat != NGHTTP2_HCAT_RESPONSE
            || frame->headers.cat != NGHTTP2_HCAT_PUSH_RESPONSE) 
        {
            return 0;
        }

        /// 新的流来了。
        int ret = 0;
        auto self = reinterpret_cast<Http2ndSession*>(user_data);

        /// 新的流被创建了，应该通知应用层
        auto & stream = self->member->streams[frame->hd.stream_id] ;

        /// 设置ID、Method、Status_Code、Request_path
        stream.id = frame->hd.stream_id;

        if (frame->headers.cat == NGHTTP2_HCAT_REQUEST)
        {
            stream.initAs(Http2ndRequest{{}});
        }
        else if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE)
        {
            stream.initAs(Http2ndResponse{{}});
        }
        if (frame->headers.cat == NGHTTP2_HCAT_PUSH_RESPONSE)
        {
            /// push_promise是frame级别的，不能和request和response混为一谈。
            self->member->onPushPromiseFrame(&stream, self, frame->push_promise);
        }

        // 用不着给stream分配指向Http2Stream的user_data指针，因为能拿到Http2Session的指针。
        // 当然，如果分配的话，效率会更高，毕竟免去一次查表。
        nghttp2_session_set_stream_user_data(session, frame->hd.stream_id,
                                    &stream); 

        if (frame->headers.cat == NGHTTP2_HCAT_PUSH_RESPONSE)
        {
            /// push_promise是frame级别的，不能和request和response混为一谈。
            self->member->onPushPromiseFrame(&stream, self, frame->push_promise);
        }
        else 
        {
            ret = self->member->onNewHead(&stream,self);
        }

        return ret;
    }

    int Http2ndSession::nghttp2OnDataChunkRecv(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data)
    {
        /**
        * @brief 
        * 虽然每个帧都会回调nghttp2OnFrameRecv，但nghttp2_frame 结构并不包含数据部分，因此需要使用
        * 此回调函数接收数据块。用户可以在此回调提前处理数据，这特别适用于流媒体。
        * 需要注意，数据不会主动push到stream里吗，如果有需要用户需在回调里自己push。
        */
        auto self = reinterpret_cast<Http2ndSession*>(user_data);
        auto stream_data = reinterpret_cast<Http2ndStream*>(nghttp2_session_get_stream_user_data(session, stream_id));
        std::string _data( reinterpret_cast<char const *>(data), len );
        int ret = self->member->onDataChunkRecv(stream_data,flags,std::move(_data),self);
        
        // stream_data->push_data(std::move(_data));
        return ret;
    }

    ssize_t Http2ndSession::nghttp2RealRecv(std::string && data)
    {
        return nghttp2RealRecv( data.c_str(), data.size() );
    }

    ssize_t Http2ndSession::nghttp2RealRecv(char const * data, size_t size)
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


}

