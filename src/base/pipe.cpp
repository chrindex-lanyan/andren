

#include "pipe.hh"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

namespace chrindex::andren::base
{

    SysUnnamedPipe::SysUnnamedPipe(bool packageMode, bool noblock)
    {
        int ret, flags = 0;
        if (packageMode)
        {
            flags = O_DIRECT;
        }
        if (noblock)
        {
            flags |= O_NONBLOCK;
        }
        if (flags)
        {
            ret = ::pipe2(m_fd, flags);
        }
        else
        {
            ret = ::pipe(m_fd);
        }
        if (ret != 0)
        {
            m_fd[0] = -1;
            m_fd[1] = -1;
        }
    }

    SysUnnamedPipe::SysUnnamedPipe(SysUnnamedPipe &&_)
    {
        m_fd[0] = _.m_fd[0];
        m_fd[1] = _.m_fd[1];
        _.m_fd[0] = -1;
        _.m_fd[1] = -1;
    }

    SysUnnamedPipe &SysUnnamedPipe::operator=(SysUnnamedPipe &&_)
    {
        if (m_fd[0] > 0 && m_fd[1] > 0)
        {
            ::close(m_fd[0]);
            ::close(m_fd[1]);
        }
        m_fd[0] = _.m_fd[0];
        m_fd[1] = _.m_fd[1];
        _.m_fd[0] = -1;
        _.m_fd[1] = -1;
    }

    SysUnnamedPipe::~SysUnnamedPipe()
    {
        if (m_fd[0] > 0 && m_fd[1] > 0)
        {
            ::close(m_fd[0]);
            ::close(m_fd[1]);
        }
    }

    int SysUnnamedPipe::PipeRDFD() const
    {
        return m_fd[0];
    }

    int SysUnnamedPipe::PipeWRFD() const
    {
        return m_fd[1];
    }

    PipeFDType SysUnnamedPipe::mode() const
    {
        return PipeFDType::READ_WRITE_FD;
    }

    PipeType SysUnnamedPipe::type() const
    {
        return PipeType::UNNAMED_PIPE;
    }

    SysNamedPipe::SysNamedPipe()
    {
        m_fd = -1;
    }

    SysNamedPipe::SysNamedPipe(const std::string &path, int mode,
                               PipeFDType rwType, bool noblock )
    {
        int ret, flags = 0;
        if (noblock)
        {
            flags |= O_NONBLOCK;
        }
        if (rwType == PipeFDType::READ_FD)
        {
            flags |= O_RDONLY;
        }
        else if (rwType == PipeFDType::WRITE_FD)
        {
            flags |= O_WRONLY;
        }
        else if (rwType == PipeFDType::READ_WRITE_FD)
        {
            flags |= O_RDWR;
        }

        ret = ::mkfifoat(AT_FDCWD, path.c_str(), mode);

        if (flags && ret > 0)
        {
            ret = ::open(path.c_str(), O_RDWR | flags);
            if (ret >= 0)
            {
                m_fd = ret;
            }
            else
            {
                m_fd = -1;
            }
        }
        else
        {
            m_fd = -1;
        }
        m_rwType = rwType;
    }

    SysNamedPipe::SysNamedPipe(SysNamedPipe &&_)
    {
        m_fd = _.m_fd;
        _.m_fd = -1;
        m_path = _.m_path;
        m_rwType = _.m_rwType;
        _.m_path = {};
    }

    SysNamedPipe &SysNamedPipe::operator=(SysNamedPipe &&_)
    {
        if (m_fd >= 0)
        {
            ::close(m_fd);
        }
        m_fd = _.m_fd;
        _.m_fd = -1;
        m_path = _.m_path;
        m_rwType = _.m_rwType;
        _.m_path = {};
    }

    SysNamedPipe::~SysNamedPipe()
    {
        if (m_fd >= 0)
        {
            ::close(m_fd);
        }
    }

    int SysNamedPipe::PipeRDFD() const
    {
        if (m_rwType == PipeFDType::READ_FD ||
            m_rwType == PipeFDType::READ_WRITE_FD)
        {
            return m_fd;
        }
        return -1;
    }

    int SysNamedPipe::PipeWRFD() const
    {
        if (m_rwType == PipeFDType::WRITE_FD ||
            m_rwType == PipeFDType::READ_WRITE_FD)
        {
            return m_fd;
        }
        return -1;
    }

    PipeFDType SysNamedPipe::mode() const
    {
        return m_rwType;
    }

    PipeType SysNamedPipe::type() const
    {
        return PipeType::FIFO_PIPE;
    }

    std::string SysNamedPipe::path() const
    {
        return m_path;
    }

    SysPipe::SysPipe()
    {
        m_handle = 0;
    }

    SysPipe::SysPipe(SysPipe &&_)
    {
        m_handle = _.m_handle;
        _.m_handle = 0;
    }

    SysPipe &SysPipe::operator=(SysPipe &&_)
    {
        m_handle = _.m_handle;
        _.m_handle = 0;
    }

    SysPipe::~SysPipe()
    {
    }

    ssize_t SysPipe::write(const std::string &data)
    {
        if (m_handle && (m_handle->mode() == PipeFDType::WRITE_FD ||
                         m_handle->mode() == PipeFDType::READ_WRITE_FD))
        {
            int fd = m_handle->PipeWRFD();
            if (fd>=0 && data.size()>0){
                ssize_t ret = 0;
                size_t count = data.size() / PIPE_BUF ;
                size_t freecount = data.size() % PIPE_BUF;
                for (size_t i =0 ; i < count  ; i++){
                    ret += ::write(fd,&data[(i * PIPE_BUF)],PIPE_BUF);
                }
                if ( freecount > 0){
                    ret += ::write(fd,&data[(count * PIPE_BUF)],freecount);
                }
                return ret;
            }
        }
        return -1;
    }

    ssize_t SysPipe::read(std::string &data)
    {
        int fd = m_handle->PipeRDFD();
        if (fd>0)
        {
            std::string _data;
            ssize_t ret=0;
            _data.resize(PIPE_BUF);
            while((ret = ::read(fd, &_data[0] , _data.size()))>0)
            {
                data += _data;
            }
            return ret;
        }
        return -1;
    }

}
