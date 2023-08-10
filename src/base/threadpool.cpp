﻿

#include <pthread.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "threadpool.hh"
#include "thread.hh"

#include <assert.h>

namespace chrindex::andren::base
{
    uint32_t hardware_vcore_count()
    {
        return (uint32_t)sysconf(_SC_NPROCESSORS_CONF);
    }

    ThreadPool::ThreadPool(uint32_t threadCount)
    {
        m_data = std::make_shared<data_t>(threadCount);

        for (int i = 0; i < threadCount; i++)
        {
            int ret = 0;
            Thread t;
            ret = t.create([data = m_data, threadno = i] /*注意保活*/
                     {
                    // 不要把锁提到for里面
                    std::unique_lock<std::mutex> locker(data->perthread_data[threadno].mut);
                    for(;;)
                    {
                        if( ! data->perthread_data[threadno].tasks.empty()) 
                        {
                            /// 任务空时自然不用退出

                            auto tasks = std::move(data->perthread_data[threadno].tasks);
                            // 在此处解锁让外部线程add task
                            locker.unlock();
                            while(tasks.size()>0){
                                tasks.front()();
                                tasks.pop();
                            }
                            locker.lock();
                        }
                        else if(data->isExit) 
                        {
                            /// 当且仅当任务为空时才判断要不要退出
                            //printf("Thread %d Will Be Exit.\n",threadno);
                            return;
                        }
                        else 
                        {
                            //printf("Thread %d Will Be Wait.\n",threadno);
                            data->perthread_data[threadno].cond.wait_for(locker,std::chrono::milliseconds(10));
                        }
                    } });
            t.detach();
            assert(ret ==0);
        }
    }

    ThreadPool::~ThreadPool()
    {
        if (m_data){
            m_data->isExit = true;
        }
    }

    ThreadPool &ThreadPool::operator=(ThreadPool &&_)
    {
        this->~ThreadPool();
        m_data = std::move(_.m_data);
        return *this;
    }

    ThreadPool::data_t::data_t(uint32_t _thread_count)
    {
        isExit = false;
        perthread_data = new perthread_data_t[_thread_count];
        thread_count = _thread_count;
    }
    ThreadPool::data_t::~data_t()
    {
        isExit = true;
        delete[] perthread_data;
    }

    ThreadPool::perthread_data_t::~perthread_data_t() 
    { 
        cond.notify_one(); 
    }

    /// ############## ThreadPoolPortable  Using std::thread ###################

    ThreadPoolPortable::ThreadPoolPortable(uint32_t threadCount)
    {
        m_data = std::make_shared<data_t>(threadCount);

        for (int i = 0; i < threadCount; i++)
        {
            int ret = 0;
            std::thread([data = m_data, threadno = i] /*注意保活*/
                     {
                    // 不要把锁提到for里面
                    std::unique_lock<std::mutex> locker(data->perthread_data[threadno].mut);
                    for(;;)
                    {
                        if( ! data->perthread_data[threadno].tasks.empty()) 
                        {
                            /// 任务空时自然不用退出
                            auto tasks = std::move(data->perthread_data[threadno].tasks);
                            // 在此处解锁让外部线程add task
                            locker.unlock();
                            while(tasks.size()>0){
                                tasks.front()();
                                tasks.pop();
                            }
                            locker.lock();
                        }
                        else if(data->isExit) 
                        {
                            /// 当且仅当任务为空时才判断要不要退出
                            return;
                        }
                        else 
                        {
                            data->perthread_data[threadno].cond.wait_for(locker,std::chrono::milliseconds(10));
                        }
                    } }).detach();
        }
    }

    ThreadPoolPortable::~ThreadPoolPortable()
    {
        if (m_data){
            m_data->isExit = true;
        }
    }

    ThreadPoolPortable &ThreadPoolPortable::operator=(ThreadPoolPortable &&_)
    {
        this->~ThreadPoolPortable();
        m_data = std::move(_.m_data);
        return *this;
    }

    ThreadPoolPortable::data_t::data_t(uint32_t _thread_count)
    {
        isExit = false;
        perthread_data = new perthread_data_t[_thread_count];
        thread_count = _thread_count;
    }
    ThreadPoolPortable::data_t::~data_t()
    {
        isExit = true;
        delete[] perthread_data;
    }

    ThreadPoolPortable::perthread_data_t::~perthread_data_t() 
    { 
        cond.notify_one(); 
    }

}
