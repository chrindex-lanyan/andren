



#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "plog.hh"

namespace chrindex ::andren::base
{

    PLog global_plog;

    PLog::PLog() : m_fd(-1), m_mode(0)
    {
    }

    PLog::~PLog()
    {
        flush();
        if (m_fd > 0)
        {
            ::close(m_fd);
        }
    }

    bool PLog::open(const std::string &path,
                    const std::string &mode)
    {
        if (mode == "sync")
        {
            m_mode = 0;
        }
        else if (mode == "async")
        {
            m_mode = 1;
        }

        if (path == "stderr")
        {
            m_fd = 2;
        }
        else if (path == "stdout")
        {
            m_fd = 1;
        }
        else
        {
            int flags = (O_APPEND | O_CREAT | O_WRONLY | O_SYNC);
            int fd = ::open(path.c_str(), flags,
                            __S_IREAD | __S_IWRITE | S_IWUSR | S_IROTH);
            if (fd > 0)
            {
                m_fd = fd;
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    PLog &PLog::print(Level level,
                      const std::string &file,
                      int line, const std::string &message)
    {
        if (m_fd <= 0)
        {
            throw "PLog is not open.";
        }

        std::string msg;
        switch (level)
        {
        case Level::LOG_INFO:
        {
            msg = "[INFO]::";
            break;
        }
        case Level::LOG_DEBUG:
        {
            msg = "[DEBUG]::";
            break;
        }
        case Level::LOG_ERR:
        {
            msg = "[ERR]::";
            break;
        }
        case Level::LOG_WARN:
        {
            msg = "[WARN]::";
            break;
        }
        default:
            break;
        }

        msg += ("[" + file + "]::" + "[" + std::to_string(line) + "] " + message + "\n");

        if (m_mode > 0)
        {
            std::unique_lock<std::mutex>(m_mut);
            m_dataQue.push_back(std::move(msg));
        }
        else
        {
            auto wsize = ::write(m_fd, &msg[0], msg.size());
            assert(wsize >= 0);
        }
    }

    void PLog::flush()
    {
        if (m_fd <= 0)
        {
            return;
        }
        if (m_mode > 0)
        {
            decltype(m_dataQue) que;
            {
                std::unique_lock<std::mutex>(m_mut);
                que = std::move(m_dataQue);
            }
            for (auto &data : que)
            {
                auto wsize = ::write(m_fd, &data[0], data.size());
                assert(wsize >= 0);
            }
        }
        ::sync();
    }

}
