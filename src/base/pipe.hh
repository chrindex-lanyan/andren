#pragma once

#include <stdint.h>
#include <string>

#include <fcntl.h>

#include "noncopyable.hpp"

namespace chrindex::andren::base
{
    class SysUnnamedPipe 
    {
    public:

        SysUnnamedPipe();

        ~SysUnnamedPipe();

        struct pipe_result_t
        {
            int rfd;
            int wfd;
            int ret;
            int eno;
        };

        static pipe_result_t pipe();

        static pipe_result_t pipe2(int flags = O_DIRECT | O_NONBLOCK);

    };

    class SysNamedPipe
    {
    public:
        SysNamedPipe();

        ~SysNamedPipe();

        struct pipe_result_t{
            int fd;
            int ret;
            int eno;
        };

        static pipe_result_t mkfifo(const std::string &path, int mode = S_IRWXU  );

        static pipe_result_t mkfifoat(int dirfd ,const std::string &path,  int mode);

        static pipe_result_t open(const std::string &path, int flags  , int mode  );

        static pipe_result_t open(const std::string &path, int flags  );

        static int access(const std::string &path, int type);

    };

    class SysPipe 
    {
    public:
        SysPipe();
        ~SysPipe();

        static ssize_t write(int fd , const std::string & data);

        static ssize_t read(int fd, std::string &data);

        static void close(int fd);

        static int fcntl(int fd , int cmd , ... );

    private:

    };

}
