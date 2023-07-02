


#include <pthread.h>

#include "threadpool.hh"
#include "thread.hh"

namespace chrindex::andren::base
{
    int32_t hardware_vcore_count()
    {
        return pthread_getconcurrency();
    }

    ThreadPool::ThreadPool(int32_t threadCount)
        {
            m_data = std::make_shared<data_t>(threadCount);
            m_data->isExit=false;
           
            for(int i = 0 ; i < threadCount ; i++)
            {
                Thread t;
                t.create([data = m_data, threadno = i] /*注意保活*/ 
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
                            break;
                        }
                        else 
                        {
                            data->perthread_data[threadno].cond.wait(locker);
                        }
                    }
                });
                t.detach();
            }
        }

        ThreadPool::~ThreadPool()
        {
            if(!m_data)
            {
                return ;
            }
            m_data->isExit=true;
            for (uint32_t i =0 ; i< m_data->thread_count;i++){
                m_data->perthread_data[i].cond.notify_all();
            }
        }

}
