#define _CRT_SECURE_NO_WARNINGS

//#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "plog.hh"

namespace chrindex ::andren::base
{

    PLog global_plog;

    PLog::PLog() : m_fd(0), m_mode(0)
    {
    }

    PLog::~PLog()
    {
        flush();
        if (m_fd != 0)
        {
            ::fclose(m_fd);
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
            m_fd = stderr;
        }
        else if (path == "stdout")
        {
            m_fd = stdout;
        }
        else
        {
            FILE* fd = fopen(path.c_str(), "w+");
            if (fd != 0)
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
        if (m_fd ==0)
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
            auto wsize = ::fwrite(&msg[0], msg.size(),1, m_fd);
            assert(wsize >= 0);
        }
        return *this;
    }

    void PLog::flush()
    {
        if (m_fd == 0)
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
                auto wsize = ::fwrite(&data[0], data.size(), 1, m_fd);
                assert(wsize >= 0);
            }
        }
        //::sync();
    }

}
