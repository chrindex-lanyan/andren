
#include "schedule.hh"

#include <cmath>
#include <unordered_map>



namespace chrindex::andren::network
{

    Schedule::Schedule(int threadSize):m_threadSize(threadSize){}
    
    Schedule::Schedule()
    {
        m_threadSize = 1;
    }

    int32_t Schedule::doSchedule(std::string const & key) const
    {
        std::hash<std::string> hasher;
        size_t hv = hasher(key);
        return hv % m_threadSize;
    }

    int32_t Schedule::doSchedule(int64_t key) const
    {
        uint64_t v = std::abs(key);
        return v % m_threadSize;
    }

    int32_t Schedule::doSchedule(uint64_t key) const
    {
        return key % m_threadSize;
    }

}