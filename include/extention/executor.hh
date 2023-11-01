#pragma once

#include <functional>
#include <memory>


#include "task_distributor.hh"
#include "schedule.hh"


namespace chrindex::andren::network
{
    class Executor : public base::noncopyable
    {
    public :
        Executor();
        Executor(uint32_t nthreads);
        Executor(Executor &&) noexcept;
        ~Executor();

        bool valid()const ;

        uint32_t threadCount() const;

        bool addTask(int32_t key , std::function<void(Executor *)> task);

        bool addTask(uint32_t threadindex , std::function<void(Executor * , uint32_t threadIndex)> task);

        bool addTask_ASAP(int32_t key , std::function<void(Executor *)> task);

        bool addTask_ASAP(uint32_t threadindex , std::function<void(Executor * , uint32_t threadIndex)> task);

        bool addTaskRandom(std::function<void(Executor *)> task);

        bool addTaskRandom_ASAP(std::function<void(Executor *)> task);

        void operator=(Executor &&);

        bool operator==(Executor &) const;

        std::thread::id threadId(uint32_t index)const;

        void shutdown_all();

    private :
        Schedule m_schedule;
        std::shared_ptr<TaskDistributor> m_distributor;
    };


}
