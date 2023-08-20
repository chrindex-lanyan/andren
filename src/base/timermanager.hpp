#pragma once

#include <vector>
#include <functional>
#include <stdint.h>


namespace chrindex::andren::base
{
    /// @brief 一个勉强能用的定时器管理器
    /// 示例： TimerManger<MinHeap<BinaryHeap<uint64_t,
    ///         KVPair<uint64_t,std::function<bool()>>>>> timerManager;
    /// 这是个二叉堆的实例化。
    /// @tparam MinHeapTy
    template <typename MinHeapTy>
    class TimerManger
    {
    public:
        using Interval = typename MinHeapTy::HeapTy::KTy;
        using Task = typename MinHeapTy::HeapTy::VTy::VTy;
        using TimerManger_t = TimerManger<MinHeapTy>;

        TimerManger() {}

        TimerManger(TimerManger_t &&_)
        {
            m_minHeap = std::move(_.m_minHeap);
        }

        TimerManger(const TimerManger_t &) = delete;

        ~TimerManger() {}

        TimerManger_t &operator=(TimerManger_t &&_)
        {
            m_minHeap = std::move(_.m_minHeap);
            return *this;
        }

        void operator=(const TimerManger_t &) = delete;

        TimerManger_t &swap(TimerManger_t &_)
        {
            std::swap(m_minHeap, _.m_minHeap);
            return *this;
        }

    public:

        /// @brief 设置获取时间(MSecond)的函数
        /// @param _func 
        void setTimeGetter(std::function<uint64_t()> _func)
        {
            m_msecFunc = std::move(_func);
        }

        TimerManger_t &create_task(Interval &&msec, Task &&task)
        {
            m_minHeap.push(std::forward<Interval>(msec + m_msecFunc()),
                           KVPair<Interval, Task>(std::forward<Interval>(msec), std::forward<Task>(task)));
            return *this;
        }

        std::vector<typename MinHeapTy::HeapTy::opt_pair_t::value_type> check()
        {
            auto crrt = m_msecFunc();
            std::vector<typename MinHeapTy::HeapTy::opt_pair_t::value_type> waitList;

            while (m_minHeap.size() > 0 && m_minHeap.min_pair().value().key() <= crrt)
            {
                auto pair_value = m_minHeap.extract_min_pair().value();
                waitList.push_back(std::move(pair_value));
            }
            return waitList;
        }

        void proxy_exec(typename MinHeapTy::HeapTy::opt_pair_t::value_type &task_pair)
        {
            auto &task = task_pair.value().value();
            if (!task)
            {
                throw "Cannot Exec!!";
            }
            if (task())
            {
                create_task(std::move(task_pair.value().key()), std::move(task_pair.value().value()));
            }
        }

        void clear()
        {
            m_minHeap.clear();
        }

    private:
        MinHeapTy m_minHeap; // 用于处理任务

        /**
         * 关于保存和处理可多次执行的任务，我暂时没有想好足够好的办法。
         * 三个思路是，
         * 1、对于这些任务，使用特别的Key结构，但是这对于Heap的效率不够好；
         * 2、在check的时候，检查Value结构的flags，但会引入指针。
         * 3、强制要求任务给一个返回值，通过返回值决定要不要继续假如定时任务。
         *
         * 目前暂时采用第三种方法。
         */
        std::function<uint64_t()> m_msecFunc;
    };
}