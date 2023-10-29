

#include "../include/andren.hh"

using namespace chrindex::andren::base;

using namespace chrindex::andren::base;


struct test_await : public co_awaitable<int>
{
    // 询问其是否就绪
    virtual bool await_ready()
    {
        return false;
    }

    // 挂起时的操作。
    virtual void await_suspend(std::coroutine_handle<> handle)
    {
        fprintf(stdout,"Awaitable...Suspend.\n");
        handle.resume();
    }

    // 恢复时被调用。通常用于返回所需数据。
    virtual int await_resume()
    {
        return 0;
    }
};

co_task<void> example_1() 
{
    fprintf(stdout, "Hello World - 1 .\n");

    co_yield 0;

    fprintf(stdout, "Hello World - 2 .\n");

    co_return;
}

co_task<int> example_2() 
{
    fprintf(stdout, "Hello World - 3 .\n");

    co_yield 0;

    fprintf(stdout, "Hello World - 4 .\n");

    co_return 1;
}


co_task<void> example_3(int a)
{
    fprintf(stdout, "Hello World - 5 .\n");

    co_yield a+1;

    fprintf(stdout, "Hello World - 6 .\n");

    co_return ;
}


co_task<int> example_4(int a)
{
    fprintf(stdout, "Hello World - 7 .\n");

    co_yield a+1;

    int b = co_await test_await{};

    fprintf(stdout, "Hello World - 8 .\n");

    co_return a+b;
}






int main(int argc, char* argv[])
{
    
    auto handle1 = example_1();

    handle1.handle().resume();

    handle1.handle().resume();

    auto handle2 = example_2();

    handle2.handle().resume();

    handle2.handle().resume();

    auto handle3 = example_3(12);

    handle3.handle().resume();

    handle3.handle().resume();

    auto handle4 = example_4(23);

    handle4.handle().resume();

    handle4.handle().resume();

    return 0;
}



