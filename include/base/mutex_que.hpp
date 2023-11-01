#pragma once 

#include <deque>
#include <mutex>
#include <utility>
#include <optional>
#include <memory>
#include <memory_resource>
#include <stdint.h>

#include "noncopyable.hpp"

#include "circleque.hpp"

namespace chrindex::andren::base{

    /// <summary>
    /// 双缓冲阻塞队列，使用默认分配器。
    /// 特点：MPSC。
    /// </summary>
    /// <typeparam name="T">类型</typeparam>
    template<typename T>
    class MutexQue 
    {
    public:

        using MutexQue_Type = MutexQue<T>;

        MutexQue() {}
        ~MutexQue() {}

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
            if (m_front.empty()) [[likely]]
            {
                {
                    std::lock_guard<std::mutex>locker(m_mut);
                    m_front.swap(m_tail);
                }
                if (m_front.empty()) [[unlikely]]
                {
                    return;
                }
            }
            res = std::move(m_front.front());
            m_front.pop_front();
            return;
        }

        void takeMulti(std::deque<T>& res)
        {
            if (m_front.empty()) [[likely]]
            {
                {
                    std::lock_guard<std::mutex>locker(m_mut);
                    m_front.swap(m_tail);
                }
                if (m_front.empty()) [[unlikely]]
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

    template<typename T>
    using mutex_que = MutexQue<T>;

    /// <summary>
    /// 双缓冲阻塞队列。
    /// 特点：MPSC。
    /// 该队列使用自定义pmr分配器。
    /// </summary>
    /// <typeparam name="T">类型</typeparam>
    /// <typeparam name="_ALLOC">分配器</typeparam>
    template<typename T, typename _ALLOC , typename = typename std::enable_if<!std::is_same<_ALLOC, void>::value>::type >
    class MutexQueAlloc
    {
    public:

        using MutexQue_Type = MutexQueAlloc<T, _ALLOC>;

        MutexQueAlloc(_ALLOC* memalloc)
            : m_front(memalloc), m_tail(memalloc) { }
        ~MutexQueAlloc() {}

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
            if (m_front.empty()) [[likely]]
            {
                {
                    std::lock_guard<std::mutex>locker(m_mut);
                    m_front.swap(m_tail);
                }
                if (m_front.empty()) [[unlikely]]
                {
                    return;
                }
            }
            res = std::move(m_front.front());
            m_front.pop_front();
            return;
        }

        void takeMulti(std::pmr::deque<T>& res)
        {
            if (m_front.empty()) [[likely]]
            {
                {
                    std::lock_guard<std::mutex>locker(m_mut);
                    m_front.swap(m_tail);
                }
                if (m_front.empty()) [[unlikely]]
                {
                    return;
                }
            }
            res = std::move(m_front);
        }

    private:


    private:
        std::pmr::deque<T> m_front;
        std::pmr::deque<T> m_tail;
        std::mutex m_mut;
    };

    template<typename T, typename _ALLOC , typename = typename std::enable_if<!std::is_same<_ALLOC, void>::value>::type >
    using mutex_que_pmr = MutexQueAlloc<T, _ALLOC>;


    /// <summary>
    /// 环形双缓冲阻塞队列。
    /// 使用了环形队列作为底层数据结构。
    /// 特点：MPSC。
    /// </summary>
    /// <typeparam name="T">类型</typeparam>
    template<typename T>
    class MutexQueCircle
    {
    public:

        using MutexQueCircle_Type = MutexQueCircle<T>;

        MutexQueCircle() :m_front(128) ,m_tail(128) { }
        ~MutexQueCircle() {}

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
            if (m_front.empty()) [[likely]]
            {
                {
                    std::lock_guard<std::mutex>locker(m_mut);
                    m_front.swap(m_tail);
                }
                if (m_front.empty()) [[unlikely]]
                {
                    return;
                }
            }
            res = std::move(m_front.front());
            m_front.pop_front();
            return;
        }

    private:

        CircularQueue<int> m_front;
        CircularQueue<int> m_tail;
        std::mutex m_mut;
    };

    template<typename T>
    using mutex_circle_que = MutexQueCircle<T>;

}