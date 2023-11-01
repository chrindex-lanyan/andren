

#include "pipe.hh"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdarg.h>

namespace chrindex::andren::base
{

    SysUnnamedPipe::SysUnnamedPipe()
    {
    }

    SysUnnamedPipe::~SysUnnamedPipe()
    {
    }

    SysUnnamedPipe::pipe_result_t SysUnnamedPipe::pipe()
    {
        pipe_result_t result;

        result.ret = ::pipe((int *)&result);
        if (result.ret)
        {
            result.eno = errno;
        }

        return result;
    }

    SysUnnamedPipe::pipe_result_t SysUnnamedPipe::pipe2(int flags)
    {
        pipe_result_t result;

        result.ret = ::pipe2((int *)&result, flags);
        if (result.ret)
        {
            result.eno = errno;
        }

        return result;
    }

    SysNamedPipe::SysNamedPipe()
    {
    }

    SysNamedPipe::~SysNamedPipe()
    {
    }

    SysNamedPipe::pipe_result_t SysNamedPipe::mkfifo(const std::string &path, int mode)
    {
        pipe_result_t result;

        result.ret = ::mkfifo(path.c_str(), mode);
        if (result.ret)
        {
            result.eno = errno;
        }

        return result;
    }

    SysNamedPipe::pipe_result_t SysNamedPipe::mkfifoat(int dirfd, const std::string &path, int mode)
    {
        pipe_result_t result;

        result.ret = ::mkfifoat(dirfd, path.c_str(), mode);
        if (result.ret)
        {
            result.eno = errno;
        }

        return result;
    }

    SysNamedPipe::pipe_result_t SysNamedPipe::open(const std::string &path, int flags)
    {

        pipe_result_t result;

        result.ret = ::open(path.c_str(), flags);
        if (result.ret<=0)
        {
            result.eno = errno;
        }else {
            result.fd = result.ret;
            result.ret = 0;
        }
    
        return result;
    }

    SysNamedPipe::pipe_result_t SysNamedPipe::open(const std::string &path, int flags, int mode)
    {

        pipe_result_t result;

        result.ret = ::open(path.c_str(), flags, mode);
        if (result.ret<=0)
        {
            result.eno = errno;
        }else {
            result.fd = result.ret;
            result.ret = 0;
        }

        return result;
    }

    int SysNamedPipe::access(const std::string &path, int type)
    {
        return ::access(path.c_str(),type);
    }

    SysPipe::SysPipe()
    {
        //
    }

    SysPipe::~SysPipe()
    {
    }

    ssize_t SysPipe::write(int fd, const std::string &data)
    {
        if (fd >= 0 && data.size() > 0)
        {
            ssize_t ret = 0;
            size_t count = data.size() / PIPE_BUF;
            size_t freecount = data.size() % PIPE_BUF;
            for (size_t i = 0; i < count; i++)
            {
                ret += ::write(fd, &data[(i * PIPE_BUF)], PIPE_BUF);
            }
            if (freecount > 0)
            {
                ret += ::write(fd, &data[(count * PIPE_BUF)], freecount);
            }
            return ret;
        }
        return -1;
    }

    ssize_t SysPipe::read(int fd, std::string &data)
    {
        if (fd > 0)
        {
            ssize_t ret = 0;
            data.resize(PIPE_BUF);
            if ((ret = ::read(fd, &data[0], data.size())) > 0)
            {
                data.resize(ret);
            }else {
                data.clear();
            }
            return ret;
        }
        return -1;
    }

    void SysPipe::close(int fd)
    {
        ::close(fd);
    }

    int SysPipe::fcntl(int fd, int cmd, ...)
    {
        va_list va;
        va_start(va, cmd);
        int ret = ::fcntl(fd, cmd, va);
        va_end(va);
        return ret;
    }

}
