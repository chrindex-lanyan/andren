
#include "../include/andren.hh"


#define SHARED_MEM_NAME "/my_shared_memory"
#define SHARED_MEM_SIZE sizeof(pthread_mutex_t)

using namespace chrindex::andren::base;

int test_shmmutex()
{   
    ShmMutex<ShmOwner> shmMutex;

    shmMutex = std::move(ShmMutex<ShmOwner>(SHARED_MEM_NAME));

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

    shmMutex = std::move(ShmMutex<ShmReference>(SHARED_MEM_NAME));

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


int test_shmem()
{
    int pid =0;
    if ((pid=fork())==0)
    {
        // child
        test_refshmutex();
    }
    else {
        test_shmmutex();
    }
    return 0;
}

int main(int argc, char ** argv)
{
    test_shmem();
    return 0;
}


