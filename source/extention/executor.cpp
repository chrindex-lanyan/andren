#include "executor.hh"
#include "task_distributor.hh"
#include <memory>


namespace chrindex::andren::network
{
    Executor::Executor():
        m_schedule(0),m_distributor(nullptr)
    {
        //
    }

    Executor::Executor(uint32_t nthreads) : 
        m_schedule(nthreads),
        m_distributor(std::make_shared<TaskDistributor>(nthreads))
    {
        m_distributor->start();
    }

    Executor::Executor(Executor && e) noexcept
    {
        m_schedule = std::move(e.m_schedule);
        m_distributor = std::move(e.m_distributor);
    }

    Executor::~Executor()
    {
        shutdown_all();
    }

    void Executor::shutdown_all()
    {
        if (m_distributor)
        {
            m_distributor->shutdown();
        }
    }

    bool Executor::valid()const 
    {
        return m_distributor && !m_distributor->isShutdown();
    }

    bool Executor::addTask(int key , std::function<void(Executor *)> task)
    {
        return m_distributor && m_distributor->addTask([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(0UL + key));
    }

    bool Executor::addTask(uint32_t threadindex , std::function<void(Executor * , uint32_t threadIndex)> task)
    {
        return m_distributor && m_distributor->addTask([threadindex, t = std::move(task) ,this]()
        {
            t(this,threadindex);
        },threadindex);
    }

    bool Executor::addTask_ASAP(int32_t key , std::function<void(Executor *)> task)
    {
        return m_distributor && m_distributor->addTask_ASAP([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(0UL + key));
    }

    bool Executor::addTask_ASAP(uint32_t threadindex , std::function<void(Executor * , uint32_t threadIndex)> task)
    {
        return m_distributor && m_distributor->addTask_ASAP([threadindex, t = std::move(task) ,this]()
        {
            t(this,threadindex);
        },threadindex);
    }

    bool Executor::addTaskRandom(std::function<void(Executor *)> task)
    {
        return m_distributor && m_distributor->addTask([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(reinterpret_cast<uint64_t>(&task)));
    }

    bool Executor::addTaskRandom_ASAP(std::function<void(Executor *)> task)
    {
        return m_distributor && m_distributor->addTask_ASAP([this, t = std::move(task)]()
        {
            t(this);
        }, m_schedule.doSchedule(reinterpret_cast<uint64_t>(&task)));
    }

    void Executor::operator=(Executor && e)
    {
        this->~Executor();
        m_schedule = std::move(e.m_schedule);
        m_distributor = std::move(e.m_distributor);
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
        return m_distributor->threadId(index);
    }

}