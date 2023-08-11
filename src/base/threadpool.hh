#pragma once

#include <memory>
#include <stdint.h>
#include <sys/types.h>
#include <type_traits>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <atomic>

#include "scoperun.hpp"

namespace chrindex::andren::base
{
/// @brief 获取处理器虚拟线程数
    /// @return
    uint32_t hardware_vcore_count();

    /// @brief 简单的线程池
    /// 使用pthread的包装Thread
    class ThreadPool
    {
    public:
        ThreadPool() = default;

        ThreadPool(ThreadPool &&) = default;

        ThreadPool(uint32_t threadCount);

        ~ThreadPool();

        ThreadPool & operator=(ThreadPool && _);

        template <typename T>
        auto exec(T &&task , uint32_t threadno) -> bool
        {
            if (!m_data || threadno >= m_data->thread_count || m_data->isExit)
            {
                return false;
            }
            SCOPE_RUN(std::lock_guard, m_data->perthread_data[threadno].mut,
                      {
                          m_data->perthread_data[threadno].tasks.emplace(std::forward<T>(task));
                      });
            m_data->perthread_data[threadno].cond.notify_one();
            return true;
        }

        bool valid () const {return bool(m_data) && !m_data->isExit;}

        bool notifyThread(uint32_t index);

    private:

        struct perthread_data_t
        {
            std::mutex mut;
            std::condition_variable cond;
            std::queue<std::function<void()>> tasks;

            perthread_data_t()= default;
            ~perthread_data_t();
        };

        struct data_t
        {
            data_t(uint32_t _thread_count);
            ~data_t();
            std::atomic<bool> isExit;
            perthread_data_t* perthread_data;
            uint32_t thread_count;
        };

        std::shared_ptr<data_t> m_data;
    };

    
    /// @brief 简易线程池
    /// 使用pthread的标准库包装std::thread 
    class ThreadPoolPortable
    {
    public:
        ThreadPoolPortable() = default;

        ThreadPoolPortable(ThreadPoolPortable &&) = default;

        ThreadPoolPortable(uint32_t threadCount);

        ~ThreadPoolPortable();

        ThreadPoolPortable & operator=(ThreadPoolPortable && _);

        template <typename T>
        auto exec(T &&task , uint32_t threadno) -> bool
        {
            if (!m_data || threadno >= m_data->thread_count || m_data->isExit)
            {
                return false;
            }
            SCOPE_RUN(std::lock_guard, m_data->perthread_data[threadno].mut,
                      {
                          m_data->perthread_data[threadno].tasks.emplace(std::forward<T>(task));
                      });
            m_data->perthread_data[threadno].cond.notify_one();
            return true;
        }

        bool valid () const {return bool(m_data) && !m_data->isExit ;}

        bool notifyThread(uint32_t index);

    private:

        struct perthread_data_t
        {
            std::mutex mut;
            std::condition_variable cond;
            std::queue<std::function<void()>> tasks;

            perthread_data_t()= default;
            ~perthread_data_t();
        };

        struct data_t
        {
            data_t(uint32_t _thread_count);
            ~data_t();
            std::atomic<bool> isExit;
            perthread_data_t* perthread_data;
            uint32_t thread_count;
        };

        std::shared_ptr<data_t> m_data;
    };

}