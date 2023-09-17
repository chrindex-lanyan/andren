#pragma once

#include "../base/andren_base.hh"


#include <functional>


namespace chrindex::andren::network
{
    /// 线程计划
    /// 根据Key分配固定的线程
    class Schedule
    {
    public :
        Schedule();
        Schedule(int threadSize);
        ~Schedule()= default;

        int32_t doSchedule(std::string const & key) const;

        int32_t doSchedule(int64_t key) const;

        int32_t doSchedule(uint64_t key) const;

        uint32_t threads() const;

    private :

        int m_threadSize;

    };


}