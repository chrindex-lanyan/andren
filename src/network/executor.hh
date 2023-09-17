#pragma once

#include "../base/andren_base.hh"

#include "task_distributor.hh"

#include "schedule.hh"
#include <functional>
#include <memory>


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

        bool addTask(int key , std::function<void(Executor *)> task);

        bool addTask(std::string const & key , std::function<void(Executor *)> task);

        bool addTaskRandom(std::function<void(Executor *)> task);

        void operator=(Executor &&);

        bool operator==(Executor &) const;

    private :
        Schedule m_schedule;
        std::unique_ptr<TaskDistributor> m_ev;

    };


}
