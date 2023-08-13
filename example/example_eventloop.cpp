#include "../src/network/andren_network.hh"


using namespace chrindex::andren;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> g_exit;



int test_Eventloop()
{
    g_exit = 0;
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   genout("\n准备退出....\n");
                   g_exit = 1;
               }) == SIG_ERR)
    {
        errout("Cannot registering signal handler");
        return -3;
    }

    std::shared_ptr<network::EventLoop> ev = std::make_shared<network::EventLoop>(4);
    bool bret = false;

    bret = ev->start();
    assert(bret);

    for (int i = 0 ; i < 100 && g_exit==0 ; i++ )
    {
        bret = ev->addTask([i]()
        {
            genout ("Hello World. -- [%d]\n",i);
        },
        network::EventLoopTaskType::IO_TASK);
        assert(bret);
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    bret = ev->addTask([]()
    {
        genout("\nuse `ctrl + c` to exit.\n");
    },
    network::EventLoopTaskType::IO_TASK);

    while(1)
    {
        if (g_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}


int main()
{
    test_Eventloop();
    return 0;
}


