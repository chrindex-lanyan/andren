#pragma once

#include <stdint.h>
#include <string>

#include "noncopyable.hpp"

namespace chrindex::andren::base
{
    enum class PipeFDType : int
    {
        READ_FD = 0,
        WRITE_FD = 1,
        READ_WRITE_FD = 2,
    };

    enum class PipeType: int 
    {
        UNNAMED_PIPE = 1,
        FIFO_PIPE = 2,
    };

    struct PipeHandle
    {
        PipeHandle (){}
        virtual ~PipeHandle(){}
        virtual PipeFDType mode() const = 0;
        virtual PipeType type() const = 0;
        virtual int PipeRDFD() const= 0;
        virtual int PipeWRFD() const =0;
    };

    class SysUnnamedPipe : public PipeHandle , public noncopyable
    {
    public:

        SysUnnamedPipe(bool packageMode = false, bool noblock = false );

        SysUnnamedPipe(SysUnnamedPipe &&);

        SysUnnamedPipe & operator=(SysUnnamedPipe &&);

        ~SysUnnamedPipe();

        int PipeRDFD() const override;

        int PipeWRFD() const override;  

        PipeFDType mode() const override;

        PipeType type() const override;

    private:
        int m_fd[2];
    };

    class SysNamedPipe: public PipeHandle , public noncopyable
    {
    public:
        SysNamedPipe();

        SysNamedPipe(const std::string &path ,int mode , PipeFDType rwType, bool noblock = false);

        SysNamedPipe(SysNamedPipe &&);

        SysNamedPipe & operator=(SysNamedPipe &&);

        ~SysNamedPipe();

        
        int PipeRDFD() const override;

        int PipeWRFD() const override;  

        PipeFDType mode() const override;

        PipeType type() const override;

        std::string path()const ;

    private:

        int m_fd;
        std::string m_path;
        PipeFDType m_rwType;
    };

    class SysPipe : public noncopyable
    {
    public:
        SysPipe();
        SysPipe(SysPipe &&);

        SysPipe & operator=(SysPipe &&);
        ~SysPipe();

        ssize_t write(const std::string & data);

        ssize_t read(std::string &data);

    private:

        PipeHandle *m_handle;

    };

}
