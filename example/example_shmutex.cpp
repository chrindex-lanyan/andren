
#include "../include/andren.hh"


#define SHARED_MEM_NAME "/my_shared_mutex"

using namespace chrindex::andren::base;

int test_shmmutex_owner()
{   
    ShmMutex<ShmOwner> shmMutex;

    shmMutex = ShmMutex<ShmOwner>(SHARED_MEM_NAME);

    if (shmMutex.valid())
    {
        shmMutex.scope_run([](int a, int b){
            // 在此进行需要同步的操作
            fprintf(stdout, "sleep 10sec ...\n");
            sleep(5);
            fprintf (stdout ,"[npsm01] val =%d.\n",a+b);
        },1,2);
    }
    else 
    {
        fprintf(stdout,"cannot initial shared mutex.errno = %d\n",shmMutex.lastErrno());
    }

    return 0;
}

int test_refshmutex()
{
    ShmMutex<ShmReference> shmMutex;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    shmMutex = ShmMutex<ShmReference>(SHARED_MEM_NAME);

    if (shmMutex.valid())
    {
        shmMutex.scope_run([](int a, int b){
            // 在此进行需要同步的操作
            fprintf (stdout ,"[npsm02] val =%d.\n",a+b);
        },3,4);
    }
    else 
    {
        fprintf(stdout,"cannot open shared mutex.errno = %d\n",shmMutex.lastErrno());
    }

    return 0;
}


int test_shmemmutex()
{
    int pid =0;
    if ((pid=fork())==0)
    {
        // child
        test_refshmutex();
    }
    else {
        test_shmmutex_owner();
    }
    return 0;
}

int main(int argc, char ** argv)
{
    test_shmemmutex();
    return 0;
}


