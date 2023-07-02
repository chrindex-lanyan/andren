#pragma once

#include <functional>
#include <string>
#include <pthread.h>

#include "noncopyable.hpp"

namespace chrindex ::andren::base
{
    /// @brief pthread 的简单包装。
    class Thread : public noncopyable
    {
    public:
        /// @brief empty construction
        Thread();

        /// @brief move construction
        /// @param _thread
        Thread(Thread &&_thread);

        /// @brief move new thread here only here thread is empty
        /// @param
        /// @return
        Thread &operator=(Thread &&);

        /// @brief destruction
        virtual ~Thread();

        /// @brief create a new pthread and run task
        /// @param  task function
        /// @return 0 is ok else failed
        int create(std::function<void()>);

        /// @brief set name for thread
        /// @param  name
        /// @return 0 is ok else failed
        int set_name(const std::string &);

        /// @brief get thread name if you were set
        /// @return 0 is ok else failed
        std::string name() const;

        /// @brief detch thread
        /// @return 0 is ok else failed
        int detach();

        /// @brief join thread
        /// @return 0 is ok else failed
        int join();

        /// @brief try join and noblock
        /// @return 0 is ok else failed
        int try_join();

        /// @brief try join with wait some times
        /// @param msec timeout
        /// @return 0 is ok else failed
        int time_join(int msec);

        /// @brief send signal to thread
        /// @param sig signal
        /// @return 0 is ok else failed
        int kill(int sig);

        /// @brief yield for caller's thread
        /// @return 0 is ok else failed
        static int yield();

        /// @brief get caller's posix thread handle
        /// @return posix thread handle
        static pthread_t self();

    protected:
    private:
        /// @brief work function
        /// @param arg arg internal
        /// @return 0 alway
        static void *work(void *arg);

        /// @brief posix thread handle
        pthread_t m_pthread;
        int m_detach;
    };

}
