﻿

#include "../include/andren.hh"

using namespace chrindex::andren::base;

std::atomic<bool> m_exit =false;

static constexpr int _limit = 6;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)


struct MyAwaitable : public awaitable_template<int>
{

    MyAwaitable() :  m_val(0) {}

    ~MyAwaitable() {}

    bool await_ready() final 
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) final 
    {
        fprintf(stdout, "MyAwaitable:: 挂起 :: I am Awaitable Func.\n");

        handle.resume();
    }

    int await_resume() final
    {
        return m_val ;
    }

private:

    int m_val;
};



coro_base<int> coroexample1(int a)
{
    int b = a;

    for (int i = 0; i < _limit / 2 ; i++)
    {
        b++;
        co_yield b;
    }
    co_return b * 2;
}



struct Awaitable_WaitExit : public awaitable_template<bool>
{
    Awaitable_WaitExit() :m_val(0) {}

    ~Awaitable_WaitExit() {}

    bool await_ready() final 
    {
        genout("Awaitable_WaitExit:: Ready :: 当前值 : %d.\n",m_val);
        if (m_val < _limit)
        {
            m_val ++ ;
            return false;
        }
        return true;
    }

    void await_suspend(std::coroutine_handle<> handle) final 
    {
        genout("Awaitable_WaitExit:: 挂起...\n");
    }

    bool await_resume() final
    {
        genout("Awaitable_WaitExit:: 恢复....\n");
        return m_exit || m_val >= _limit ;
    }

private :
    // 为了演示我加了个变量
    int m_val;

    /// .... 假设以下还有一段字段

};


int main(int argc , char ** argv)
{
    TaskRunner runner;
    coro_base<int> co{ coroexample1, 0 };

    runner.push_back( 123 , std::move(co) );

    runner.push_back( 456 , [ &runner ](int)->coro_base<int> 
        {
            coro_base<int> *p_co = 0;

            do 
            {
                if (p_co) 
                {
                    fprintf(stdout,"lambda :: corotinue example 1 is not exit!!.\n");

                    int ret = co_await MyAwaitable{};

                    co_yield ret; 
                }

                p_co = runner.find_handle(123);

            } while(p_co && p_co->handle().done() == false);

            p_co = runner.find_handle(123);

            if (p_co)
            {
                fprintf(stdout, "lambda :: corotinue example 1 is exit , return value = %d.\n",
                    p_co->handle().promise().m_value);
            }

            co_return 0;
        }, 0);

    runner.push_back(678,[&runner](int)->coro_base<int>
    {
        auto wait_exit = Awaitable_WaitExit {};
        while (1)
        {
            bool isExit = co_await wait_exit;
            if (isExit){
                runner.stop();
                co_return 0;
            }
            else 
            {
                co_yield 1;
            }
        }
        
    },0 );

    m_exit = false;
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   genout(" \n::准备退出....\n");
                   m_exit = true;
               }) == SIG_ERR)
    {
        errout("Cannot registering signal handler");
        return -3;
    }

    runner.start();

    return 0;
}




