



#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

#include "signal.hh"

using namespace chrindex::andren::base;

int test_process_parent()
{
    PosixNamedSignal nsig;
    PosixSignal psig;
    bool ret = true;

    nsig.open(
        "/testSig01", O_CREAT, [&ret](int errNO)
        {
            fprintf(stdout,"[parent] open named signal errno = %d\n",errNO);
            if (errNO == EEXIST ){
                ret = true;
            }
            else {
                ret = false;
            } },
        0611, 0);
    psig.borrow_by(&nsig);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (int i = 0; i < 5; i++)
    {
        psig.post();
        int val;
        psig.value(val);
        fprintf(stdout, "[parent] post a named signal. Current = %d.\n", val);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return 0;
}

int test_process_child()
{
    PosixNamedSignal nsig;
    PosixSignal psig;

    bool ret = true;

    nsig.open(
        "/testSig01", O_CREAT, [&ret](int errNO)
        {
            fprintf(stdout,"[child] open named signal errno = %d\n",errNO);
            if (errNO == EEXIST ){
                ret = true;
            }
            else {
                ret = false;
            } },
        0611, 0);

    psig.borrow_by(&nsig);

    for (int i = 0; i < 5; i++)
    {
        int ret = psig.timeWait(1000); // 1S
        // int ret = psig.wait(); // 1S
        int val;
        psig.value(val);
        fprintf(stdout, "[child] Value Current = %d.\n", val);
        if (ret == -1)
        {
            ret = errno;
            if (ret == ETIMEDOUT)
            {
                fprintf(stdout, "[child] wait named signal timeout.\n");
            }
            else
            {
                fprintf(stdout, "[child] wait named signal error.errno = %d\n", ret);
            }
        }
        else
        {

            fprintf(stdout, "[child] wait named signal succeeded.\n");
        }
    }

    return 0;
}

int test_named_signal()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        test_process_child();
    }
    else
    {
        test_process_parent();
    }
    return 0;
}

int test_unnamed_signal()
{
    std::atomic<int> flag_exit = 0;

    int signal_value = 0;

    PosixUnnamedSignal usig(signal_value);
    PosixSignal psig;

    psig.borrow_by(&usig);

    // sub-thread
    std::thread t([&flag_exit, &psig]()
                  {
        for (;flag_exit==0;){
            int ret = psig.timeWait(1000);
            int val ;
            psig.value(val);
            fprintf(stdout,"[Sub Thread] Value Current = %d.\n", val);
            if (ret ==0 ){
                fprintf (stdout,"[Sub Thread] Wait Unnamed Signal succeeded.\n");
            }else {
                ret = errno;
                if (ret == ETIMEDOUT){
                    fprintf(stdout,"[Sub Thread] wait unnamed signal timeout.\n");
                }else {
                    fprintf(stdout,"[Sub Thread] wait named signal error.errno = %d\n",ret);
                }
            }
        } });

    for (int i = 0; i < 5; i++)
    {
        psig.post();
        int val;
        psig.value(val);
        fprintf(stdout, "[Main Thread] post a named signal. Current = %d.\n", val);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    flag_exit = true;

    if (t.joinable())
    {
        t.join();
    }
    return 0;
}

int main(int argc, char **argv)
{
    test_named_signal();
    test_unnamed_signal();
    return 0;
}
