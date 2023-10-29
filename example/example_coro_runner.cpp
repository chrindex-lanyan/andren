

#include "../include/andren.hh"

using namespace chrindex::andren::base;

std::atomic<bool> m_exit =false;

static constexpr int _limit = 6;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)


coroutine_template<int> coroexample1(int a)
{
    int b = a;

    for (int i = 0; i < _limit / 2 ; i++)
    {
        b++;
        co_yield b;
    }
    co_return b * 2;
}




int main(int argc , char ** argv)
{
    TaskRunner runner;
    coroutine_template<int> co{ coroexample1, 0 };

    runner.push_back( 123 , std::move(co) );

    runner.push_back( 456 , [ &runner ](int)->coroutine_template<int> 
        {
            coroutine_template<int> *p_co = 0;

            do 
            {
                if (p_co) 
                {
                    fprintf(stdout,"lambda 456 :: corotinue example 1 is not exit!!.\n");
                    co_yield 0; 
                }
                p_co = runner.find_handle(123);
            } 
            while(p_co && p_co->handle().done() == false);

            p_co = runner.find_handle(123);

            if (p_co)
            {
                fprintf(stdout, "lambda 456 :: corotinue example 1 is exit , return value = %d.\n",
                    p_co->handle().promise().result());
            }

            co_return 0;
        }, 0);

    // runner.push_back(678,[&runner](int)->coroutine_template<int>
    // {
    //     auto wait_exit = Awaitable_WaitExit {};
    //     while (1)
    //     {
    //         bool isExit = co_await wait_exit;
    //         if (isExit){
    //             runner.stop();
    //             genout("lambda 678 :: return...\n");
    //             co_return 0;
    //         }
    //         else 
    //         {
    //             genout("lambda 678 :: yield...\n");
    //             co_yield 1;
    //         }
    //     }
    // },0 );

    runner.push_back(789 , [](std::string s)->coroutine_template<int>
    {
        uint64_t count = 0;

        genout("%s.\n",s.c_str());

        while(1)
        {
            count ++;
            genout("第[%lu]轮.\n",count);
            co_yield 0;
        }
        co_return 0;
    }, "lambda 789:: started" );

    runner.push_back(890 , []()->coroutine_template<int>
    {
        genout("无参.\n");
        co_return 0;
    });

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





