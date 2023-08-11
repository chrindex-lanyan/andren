

#include "eventloop.hh"

#include <algorithm>


namespace chrindex::andren::network
{
    EventLoop::EventLoop(uint32_t size)
    {
        m_size = std::max(size, 2u);
        m_bqueForTask = new base::DBuffer<Task>[m_size];
        m_exit = false;
    }

    EventLoop::~EventLoop()
    {
        shutdown();
        delete [] m_bqueForTask;
    }

    bool EventLoop::addTask(Task task, EventLoopTaskType type)
    {
        if(!m_tpool.valid() || !task)
        {
            return false;
        }

        uint64_t randmsec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock()
            .now().time_since_epoch()).count();
        int32_t randindex = randmsec%(m_size-1);
        if (type == EventLoopTaskType::IO_TASK)
        {
            m_bqueForTask[0].pushBack(std::move(task));
        }
        else if (type == EventLoopTaskType::SHCEDULE_TASK)
        {
            m_bqueForTask[1].pushBack(std::move(task));
        }
        else if (type == EventLoopTaskType::FURTURE_TASK)
        {
            m_bqueForTask[randindex].pushBack(std::move(task));
        }

        return true;
    }

    bool EventLoop::addTask(Task task, uint32_t index)
    {
        if(!m_tpool.valid() || !task)
        {
            return false;
        }
        if (index >= m_size)
        {
            index = m_size -1;
        }

        m_bqueForTask[index].pushBack(std::move(task));
        return true;
    }

    bool EventLoop::start()
    {
        m_tpool = std::move(base::ThreadPoolPortable(m_size));
        for (uint32_t i=0;i< m_size;i++)
        {
            startNextLoop(i);
        }
        return true;
    }

    void EventLoop::shutdown()
    {
        // 设置退出标识
        m_exit = true;
        // 唤醒所有的线程
        for (uint32_t i = 0; i < m_size ; i++)
        {
            m_tpool.notifyThread(i);
        }
    }

    bool EventLoop::isShutdown()const 
    {
        return m_exit;
    }

    void EventLoop::startNextLoop(uint32_t index)
    {
        if (m_exit)
        {
            return ;
        }
        m_tpool.exec([self = shared_from_this(), index]()->bool
        {
            self->work(index);
            self->startNextLoop(index);
            return true;
        },index);
    }

    void EventLoop::work(uint32_t index)
    {
        std::optional<std::deque<Task>> result;
        m_bqueForTask[index].takeMulti(result);

        if (!result.has_value())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            return ;
        }
        for (auto & task : result.value() )
        {
            if (task)
            {
                task();
            }
        }
    }
}
