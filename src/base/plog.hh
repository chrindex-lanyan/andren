#pragma once

#include <mutex>
#include <string>
#include <deque>

#include "noncopyable.hpp"

namespace chrindex ::andren::base
{
/// @brief 简单的日志打印类。
    class PLog : public noncopyable
    {
    public:
        PLog();

        ~PLog();

        /// @brief 打开日志文件
        /// @param path 日志文件path。path还可以为stderr和stdout。
        /// @param mode 同步或者异步模式。字符串为"sync","async"。
        /// @return
        bool open(const std::string &path,
                  const std::string &mode = "sync");

        enum class Level : int
        {
            LOG_INFO,
            LOG_DEBUG,
            LOG_WARN,
            LOG_ERR,
        };

        PLog &print(Level level,
                    const std::string &file,
                    int line, const std::string &message);

        void flush();

    private:
        int m_fd;
        int m_mode;
        std::mutex m_mut;
        std::deque<std::string> m_dataQue;
    };

    /// @brief  全局的PLog
    extern class PLog global_plog;

/// 包装的PLOG宏
#define PLOG(level, msg)                                   \
    {                                                      \
        global_plog.print(level, __FILE__, __LINE__, msg); \
    }

}