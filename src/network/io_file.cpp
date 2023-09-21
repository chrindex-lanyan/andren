
#include "io_file.hh"
#include "io_service.hh"
#include <memory>

namespace chrindex::andren::network
{
    io_file::io_file() : m_fd(-1)
    {
        //
    }

    io_file::io_file(std::string const & path, uint32_t flags , uint32_t mode)
    {
        immediately_open(path, flags, mode);
    }

    io_file::io_file(io_file && another)
    {
        m_fd = another.m_fd;
        another.m_fd = -1;
    }

    io_file::~io_file()
    {
        immediately_close();
    }

    bool io_file::async_open(std::function<void(io_file *self, int32_t ret)> onOpen , IOService & ioservice  ,
        std::string const & path, int dir_fd, uint32_t flags , uint32_t mode )
    {
        io_context context;
        uint64_t uid = base::create_uid_u64();

        context.req_context->general.req = network::io_request::OPEN;
        context.req_context->ioData.file.dfd = dir_fd;
        context.req_context->general.uid = uid;
        context.req_context->general.flags = flags;
        context.req_context->ioData.file.mode = mode;
        memcpy(context.req_context->ioData.file.path,path.c_str(), 
            std::min(sizeof(context.req_context->ioData.file.path), path.size())); 
        context.req_context->ioData
            .file.path[std::min(sizeof(context
                .req_context->ioData.file.path),path.size())] = 0;

        context.onEvents = [onOpen = std::move(onOpen)](io_context * pcontext , int32_t cqe_res)->bool
        {
            io_file iofile;

            iofile.resume(cqe_res);
            if (onOpen)
            {
                onOpen(&iofile,cqe_res);
            }
            return true;
        };
        return ioservice.submitRequest(uid, std::move(context));
    }

    bool io_file::async_write(std::function<void (io_file *self, ssize_t wrsize)> onWrite , 
        IOService & ioservice ,std::string data)
    {
        io_context context;
        uint64_t uid = base::create_uid_u64();
        std::string *pdata =0;

        context.userdata = std::make_shared<std::string>(std::move(data));
        pdata = reinterpret_cast<std::string*>(context.userdata.get());
        context.req_context->general.req = network::io_request::WRITE;
        context.req_context->general.fd = take_handle();
        context.req_context->general.uid = uid;
        context.req_context->general.size = pdata->size();
        context.req_context->ioData.bufData.buf_ptr = &(*pdata)[0];
        
        context.onEvents = [onWrite = std::move(onWrite)](io_context * pcontext, int32_t cqe_res)->bool
        {
            io_file iofile;
            
            iofile.resume(pcontext->req_context->general.fd);
            if(onWrite)
            {
                onWrite(&iofile,cqe_res);
            }
            return true;
        };
        return ioservice.submitRequest(uid, std::move(context));
    }

    bool io_file::async_read(std::function<void(io_file *self, std::string && data , size_t rdsize )> onRead ,
        IOService & ioservice)
    {
        io_context context;
        uint64_t uid = base::create_uid_u64();

        context.userdata = std::make_unique<std::string>();
        context.req_context->general.req = network::io_request::READ;
        context.req_context->general.uid = uid;
        context.req_context->general.fd = take_handle();

        context.onEvents = [onRead = std::move(onRead)](io_context * pcontext, int32_t cqe_res)->bool
        {
            ssize_t readsize =0;
            std::string * pstr = reinterpret_cast<std::string*>(pcontext->userdata.get());

            if (cqe_res >0 && cqe_res >= sizeof(pcontext->req_context->ioData.bufData.buf))
            {
                pstr->append(std::string (pcontext->req_context->ioData.bufData.buf, cqe_res));
                return false;
            }

            io_file iofile;

            iofile.resume(pcontext->req_context->general.fd);
            if (onRead)
            {
                readsize = pstr->size();
                onRead(&iofile, std::move(*pstr),readsize);
            }
            return true;
        };

        return ioservice.submitRequest(uid, std::move(context));
    }

    bool io_file::async_close(std::function<void (io_file * self)> onClose , 
        IOService & ioservice )
    {
        io_context context;
        uint64_t uid = base::create_uid_u64();

        context.req_context->general.req = network::io_request::CLOSE;
        context.req_context->general.uid = uid;
        context.req_context->general.fd = take_handle();
        context.onEvents = [onClose = std::move(onClose)](io_context * pcontex , int32_t cqe_res)->bool
        {
            io_file iofile;

            iofile.resume(pcontex->req_context->general.fd);
            if (onClose)
            {
                onClose(&iofile);
            }
            return true;
        };
        return ioservice.submitRequest(uid, std::move(context));
    }

    int io_file::immediately_open(std::string const & path, uint32_t flags , uint32_t mode)
    {
        base::File _f ;
        int ret = _f.open(path, flags, mode);
        m_fd = _f.take_handle();
        return ret;
    }

    ssize_t io_file::immediately_write(char const * data , size_t size)
    {
        base::File _f (m_fd);
        ssize_t ret = _f.write(data, size);
        _f.take_handle();
        return ret;
    }

    ssize_t io_file::immediately_read(std::string & data)
    {
        base::File _f(m_fd);
        ssize_t ret =_f.readToBuf(data);
        _f.take_handle();
        return ret;
    }

    void io_file::immediately_close()
    {
        base::File(m_fd).close();
    }

    bool io_file::valid () const 
    {
        return m_fd >0 ;
    }

    int io_file::take_handle()
    {
        int fd = m_fd;
        m_fd = -1;
        return fd;
    }

    int io_file::handle() const 
    {
        return m_fd;
    }

    void io_file::resume(int fd)
    {
        m_fd = fd;
    }

}

