#include <deque>
#include <mutex>
#include <utility>
#include <optional>

#include <stdint.h>

#include "noncopyable.hpp"

namespace chrindex::andren::base{


/// <summary>
    /// 单线读写队列。(多线程生产者单线程消费者)
    /// 不管有几个消费者，他们必须同属一个线程。
    /// 不管有几个生产者，他们无须同属一个线程。
    /// </summary>
    /// <typeparam name="T">类型</typeparam>
    template<typename T>
    class DBuffer 
    {
    public:

        using DBuffer_Type = DBuffer<T>;

        DBuffer() {}
        ~DBuffer() {}

        /// @brief 丢回一堆元素
        /// @param deq 
        void appendFromDeque(std::deque<T>&& deq)
        {
            std::lock_guard<std::mutex> locker(m_mut);
            m_tail.insert(m_tail.begin(), std::make_move_iterator(deq.begin()), std::make_move_iterator(deq.end()));
        }

        /// @brief 丢回一堆元素
        /// @param deq 
        void appendFromDeque(std::deque<T> const& deq)
        {
            std::lock_guard<std::mutex> locker(m_mut);
            m_tail.insert(m_tail.begin(), deq.begin(), deq.end());
        }

        /// @brief 丢回一个元素
        /// @param t 
        void pushBack(T const& t)
        {
            std::lock_guard<std::mutex> locker(m_mut);
            m_tail.push_back(t);
        }

        /// @brief 丢回一个元素
        /// @param t 
        void pushBack(T&& t)
        {
            std::lock_guard<std::mutex> locker(m_mut);
            m_tail.emplace_back(std::forward<T>(t));
        }

        /// @brief 尝试取出一个元素
        /// @return 
        std::optional<T> takeOne()
        {
            std::optional<T> res;
            takeOne(res);
            return res;
        }

        /// @brief 尝试取出一个元素
        /// @param res 
        void takeOne(std::optional<T>& res)
        {
            if (m_front.empty()) 
            {
                {
                    std::lock_guard<std::mutex>locker(m_mut);
                    m_front.swap(m_tail);
                }
                if (m_front.empty()) 
                {
                    return;
                }
            }
            res = std::move(m_front.front());
            m_front.pop_front();
            return;
        }

        void takeMulti(std::optional<std::deque<T>>& res)
        {
            if (m_front.empty()) 
            {
                {
                    std::lock_guard<std::mutex>locker(m_mut);
                    m_front.swap(m_tail);
                }
                if (m_front.empty()) 
                {
                    return;
                }
            }
            res = std::move(m_front);
        }

    private:


    private:
        std::deque<T> m_front;
        std::deque<T> m_tail;
        std::mutex m_mut;
    };

}