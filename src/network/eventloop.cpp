﻿

#include "eventloop.hh"

#include <algorithm>
#include <condition_variable>
#include <mutex>


namespace chrindex::andren::network
{
    EventLoop::EventLoop(uint32_t size)
    {
        m_size = std::max(size, 2u);
        m_tpool = nullptr;
        m_cvmut = new std::mutex [m_size];
        m_cv = new std::condition_variable [m_size];
        m_bqueForTask = new base::DBuffer<Task>[m_size];
        m_exit = false;
    }

    EventLoop::~EventLoop()
    {
        shutdown();
        delete [] m_tpool;
        delete [] m_bqueForTask;
        delete [] m_cv;
        delete [] m_cvmut;
    }

    bool EventLoop::addTask(Task task, EventLoopTaskType type)
    {
        bool bret = false;

        if (type == EventLoopTaskType::IO_TASK)
        {
            bret = addTask(std::move(task), 0 );
        }
        else if (type == EventLoopTaskType::SHCEDULE_TASK)
        {
            bret = addTask(std::move(task), 1 );
        }
        else if (type == EventLoopTaskType::FURTURE_TASK)
        {
            uint64_t randmsec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock()
                .now().time_since_epoch()).count();
            int32_t randindex = randmsec%(m_size-1);
            bret = addTask(std::move(task), randindex );
        }

        return bret;
    }

    bool EventLoop::addTask(Task task, uint32_t index)
    {
        if(m_tpool == nullptr || !task)
        {
            return false;
        }

        if (index >= m_size)
        {
            index = m_size -1;
        }

        m_bqueForTask[index].pushBack(std::move(task));
        m_cv[index].notify_one();
        return true;
    }

    bool EventLoop::start()
    {
        m_exit = false;        
        m_tpool = new std::thread[m_size];
        for (uint32_t i=0;i< m_size;i++)
        {
            m_tpool[i] = std::thread([i ,this, self = shared_from_this()]()
            {
                startNextLoop(i);
            });
        }
        return true;
    }

    void EventLoop::shutdown()
    {
        // 设置退出标识
        m_exit = true;
        // 唤醒所有的线程
        for (uint32_t i = 0; i < m_size && m_tpool; i++)
        {
            m_cv[i].notify_all();
            if (m_tpool[i].joinable())
            {
                m_tpool[i].join();
            }
        }
    }

    bool EventLoop::isShutdown()const 
    {
        return m_exit;
    }

    void EventLoop::startNextLoop(uint32_t index)
    {
        while(!m_exit)
        {
            work(index);
        }
    }

    void EventLoop::work(uint32_t index)
    {
        //std::optional<std::deque<Task>> result;
        std::deque<Task> result;
        m_bqueForTask[index].takeMulti(result);

        if(result.empty())
        {
            std::unique_lock<std::mutex>locker(m_cvmut[index]);
            m_cv[index].wait_for(locker,std::chrono::microseconds(100));
            return ;
        }
        //for (auto & task : result.value() )
        for (auto & task : result)
        {
            if (task)
            {
                task();
            }
        }
    }
}
