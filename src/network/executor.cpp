#include "executor.hh"
#include "task_distributor.hh"
#include <memory>


namespace chrindex::andren::network
{
    Executor::Executor():
        m_schedule(0),m_ev(nullptr)
    {
        //
    }

    Executor::Executor(uint32_t nthreads) : 
        m_schedule(nthreads),
        m_ev(std::make_unique<TaskDistributor>(nthreads))
    {
        m_ev->start();
    }

    Executor::Executor(Executor && e) noexcept
    {
        m_schedule = std::move(e.m_schedule);
        m_ev = std::move(e.m_ev);
    }

    Executor::~Executor()
    {
        m_ev->shutdown();
    }

    bool Executor::valid()const 
    {
        return m_ev && !m_ev->isShutdown();
    }

    bool Executor::addTask(int key , std::function<void(Executor *)> task)
    {
        return m_ev && m_ev->addTask([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(0UL + key));
    }

    bool Executor::addTask(uint32_t threadindex , std::function<void(Executor * , uint32_t threadIndex)> task)
    {
        return m_ev && m_ev->addTask([threadindex, t = std::move(task) ,this]()
        {
            t(this,threadindex);
        },threadindex);
    }

    bool Executor::addTask_ASAP(int32_t key , std::function<void(Executor *)> task)
    {
        return m_ev && m_ev->addTask_ASAP([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(0UL + key));
    }

    bool Executor::addTask_ASAP(uint32_t threadindex , std::function<void(Executor * , uint32_t threadIndex)> task)
    {
        return m_ev && m_ev->addTask_ASAP([threadindex, t = std::move(task) ,this]()
        {
            t(this,threadindex);
        },threadindex);
    }

    bool Executor::addTaskRandom(std::function<void(Executor *)> task)
    {
        return m_ev && m_ev->addTask([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(reinterpret_cast<uint64_t>(&task)));
    }

    bool Executor::addTaskRandom_ASAP(std::function<void(Executor *)> task)
    {
        return m_ev && m_ev->addTask_ASAP([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(reinterpret_cast<uint64_t>(&task)));
    }

    void Executor::operator=(Executor && e)
    {
        this->~Executor();
        m_schedule = std::move(e.m_schedule);
        m_ev = std::move(e.m_ev);
    }

    bool Executor::operator==(Executor & e) const
    {
        return this == &e;
    }

    uint32_t Executor::threadCount() const
    {
        return m_schedule.threads();
    }

    std::thread::id Executor::threadId(uint32_t index)const
    {
        return m_ev->threadId(index);
    }

}