#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>
#include <initializer_list>

#include "noncopyable.hpp"
#include "scoperun.hpp"

namespace chrindex ::andren::base
{


    /// @brief 简单的阻塞队列
    /// @tparam T
    template <typename T>
    class block_que : public noncopyable
    {
    public:
        block_que()
        {
        }

        block_que(const std::initializer_list<T> &list)
        {
            std::deque<T> que;
            for (auto &val : list)
            {
                que.push_back(val);
            }
            SCOPE_RUN(std::lock_guard, m_mut, {
                m_que.swap(que);
                m_cond.notify_all();
            });
        }

        ~block_que()
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                m_cond.notify_all();
            });
        }

        void push_back(const T &t)
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                m_que.push_back(t);
                m_cond.notify_all();
            });
        }

        void push_back(T &&t)
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                m_que.push_back(std::move(t));
                m_cond.notify_all();
            });
        }

        void take_front(T &t)
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                t = m_que.front();
                m_cond.notify_all();
            });
        }

        T take_front()
        {
            T t;
            SCOPE_RUN(std::lock_guard, m_mut, {
                t = std::move(m_que.front());
                m_que.pop_front();
            });
            return t;
        }

        void pop_front()
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                m_que.pop_front();
                m_cond.notify_all();
            });
        }

        size_t size()
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                return m_que.size();
            });
        }

        void clear()
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                m_que.clear();
                m_cond.notify_all();
            });
        }

        std::deque<T> &raw_ref()
        {
            SCOPE_RUN(std::lock_guard, m_mut, {
                return m_que;
            });
        }

        std::deque<T> raw()
        {
            std::deque<T> _d;
            SCOPE_RUN(std::lock_guard, m_mut, {
                _d.swap(_d);
                m_cond.notify_all();
            });
            return _d;
        }

    private:
        std::mutex m_mut;
        std::condition_variable m_cond;
        std::deque<T> m_que;
    };
}
