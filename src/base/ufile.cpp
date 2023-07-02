


#include <unistd.h>
#include <sys/errno.h>
#include <fcntl.h>

#include "ufile.hh"

namespace chrindex ::andren::base
{
    File::File() : m_fd(-1) {}

    File::File(int fd) : m_fd(fd) {}

    File::File(File &&_file)
    {
        File().swap(_file).swap(*this);
    }

    File::~File()
    {
        close();
    }

    File &File::operator=(File &&_file)
    {
        File().swap(_file).swap(*this);
        return *this;
    }

    File::operator bool() const
    {
        return m_fd >= 0;
    }

    bool File::open(const std::string &path, int flags, int createMode)
    {
        int fd = -1;
        if (createMode < 0)
        {
            fd = ::open(path.c_str(), flags);
        }
        else {
            fd = ::open(path.c_str(), flags, createMode);
        }
        if (!(fd < 0)){
            m_fd = fd;
        }
        return !(fd < 0); 
    }

    void File::close()
    {
        if (m_fd < 0)
        {
            return;
        }
        int fd = m_fd;
        m_fd = -1;
        ::close(fd);
    }

    ssize_t File::write(const char *data, size_t size)
    {
        return ::write(m_fd, data, size);
    }

    ssize_t File::writeByBuf(const std::string &buf)
    {
        return write(buf.c_str(), buf.size());
    }

    ssize_t File::read(char *buf, size_t buf_size)
    {
        return ::read(m_fd, buf, buf_size);
    }

    ssize_t File::readToBuf(std::string &buf)
    {
        return read(&buf[0], buf.size());
    }

    File &File::swap(File &_file)
    {
        std::swap(m_fd, _file.m_fd);
        return *this;
    }

    int File::handle() const
    {
        return m_fd;
    }

    int File::fcntl(int fd, int cmd)
    {
        return ::fcntl(fd,cmd);
    }

    int File::fcntl(int fd, int cmd, long arg)
    {
        return ::fcntl(fd,cmd,arg);
    }

    int File::fcntl(int fd, int cmd, struct flock *lock)
    {
        return ::fcntl(fd,cmd,lock);
    }

    void File::sync(void)
    {
        ::sync();
    }

}
