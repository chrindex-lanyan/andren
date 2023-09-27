
#pragma once


#include <stdint.h>
#include <coroutine>
#include <stdio.h>
#include <assert.h>
#include <deque>
#include <map>


#include "corotemplate.hpp"

namespace chrindex::andren::base 
{
    class TaskRunner 
    {
    public :

        using coro_type = co_task<int>;

        TaskRunner() : m_exit(false) {}
        ~TaskRunner() { stop(); }

        void push_back(uint64_t key , coro_type && coro_task)
        {
            m_tque[key]=std::move(coro_task);
        }

        template <typename FN >
        void push_back(uint64_t key, FN && fn ) 
        {
            coro_type co = fn();
            m_tque[key]=std::move(co);
        }

        template <typename FN , typename ...ARGS>
        void push_back(uint64_t key, FN && fn , ARGS && ... args ) 
        {
            coro_type co = fn(std::forward<ARGS>(args)...);
            m_tque[key]=std::move(co);
        }

        coro_type * find_handle(uint64_t key)
        {
            auto iter = m_tque.find(key);
            if (iter != m_tque.end()) 
            {
                return  & iter->second;
            }

            iter = m_drop_que.find(key);
            if (iter != m_drop_que.end()) 
            {
                return &iter->second;
            }

            return nullptr;
        }

        bool notify_one_resume(uint64_t key)
        {
            auto p_coro = find_handle(key);
            if (p_coro)
            {
                p_coro->handle().resume();
                return true;
            }
            return false;
        }

        void start() 
        {
            m_exit = false;
            work();
        }

        void stop() 
        {
            m_exit = true;
        }

    private :


        co_task<int> real_work(int)
        {
            while (!m_exit)
            {
                m_drop_que.clear();
                for (auto iter = m_tque.begin() ; iter != m_tque.end() && m_exit != true; )
                {
                    iter->second.handle().resume();
                    if (iter->second.handle().done())
                    {
                        m_drop_que[iter->first] = std::move(iter->second);
                        iter = m_tque.erase(iter);
                    }
                    else 
                    {
                        iter++;
                    }
                }
            }
            co_return 0;
        }


        void work() 
        {
            auto main_coro = real_work(0);
            main_coro.handle().resume();
        }

    private :
        
        bool m_exit;
        std::map<uint64_t, coro_type> m_tque;
        std::map<uint64_t, coro_type> m_drop_que;
    };

}




