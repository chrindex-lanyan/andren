

#include "../include/andren.hh"

using namespace chrindex::andren::base;

std::atomic<bool> m_exit =false;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)


struct MyAwaitable : public awaitable_template<int>
{

    MyAwaitable() :  m_val(0) {}

    ~MyAwaitable() {}

    bool await_ready() const final 
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) final 
    {
        fprintf(stdout, "I am Awaitable Func.\n");

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

    for (int i = 0; i < 10; i++)
    {
        b++;
        co_yield b;
    }
    co_return b * 2;
}



struct Awaitable_WaitExit : public awaitable_template<bool>
{
    Awaitable_WaitExit() {}

    ~Awaitable_WaitExit() {}

    bool await_ready() const final 
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) final 
    {
        handle.resume();
    }

    bool await_resume() final
    {
        return m_exit ;
    }

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
                    fprintf(stdout,"corotinue example 1 is not exit!!.\n");

                    int ret = co_await MyAwaitable{};

                    co_yield ret; 
                }

                p_co = runner.find_handle(123);

            } while(p_co && p_co->handle().done() == false);

            p_co = runner.find_handle(123);

            if (p_co)
            {
                fprintf(stdout, "corotinue example 1 is exit , return value = %d.\n",
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





