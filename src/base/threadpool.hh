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
    int32_t hardware_vcore_count();

    /// @brief 简单的线程池
    class ThreadPool
    {
    public:
        ThreadPool() = default;

        ThreadPool(ThreadPool &&) = default;

        ThreadPool(int32_t threadCount);

        ~ThreadPool();

        template <typename T>
        auto exec(T &&task , uint32_t threadno) -> bool
        {
            if (!m_data && threadno >= m_data->thread_count)
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

    private:

        struct perthread_data_t
        {
            std::mutex mut;
            std::condition_variable cond;
            std::queue<std::function<void()>> tasks;
        };

        struct data_t
        {
            data_t(int _thread_count){
                isExit = false;
                perthread_data = new perthread_data_t [_thread_count];
                thread_count = _thread_count;
            }
            ~data_t(){
                delete [] perthread_data;
            }
            std::atomic<bool> isExit;
            perthread_data_t* perthread_data;
            uint32_t thread_count;
        };

        std::shared_ptr<data_t> m_data;
    };
}