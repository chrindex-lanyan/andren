

#include "../include/andren.hh"

#define SHARED_MEM_NAME "/my_shared_memory"

using namespace chrindex::andren::base;

struct MyData
{
    char data[1024];
    pthread_mutex_t mutex;
};

int test_shmemowner()
{
    SharedMem<MyData> shmem;
    std::string msg = "hello world";
    int ret =0;

    shmem = std::move(SharedMem<MyData>(SHARED_MEM_NAME, true));
    if (shmem.valid())
    {
        // clear
        MyData * pdata = shmem.reference();
        memset(pdata->data , 0, sizeof(MyData::data));

        fprintf(stdout,"[parent] clear shared memory.\n");

        // 初始化互斥锁
        auto shared_mutex = & pdata->mutex;
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        ret = pthread_mutex_init(shared_mutex, &mutex_attr);

        fprintf(stdout,"[parent] prepare lock mutex.\n");

        pthread_mutex_lock(shared_mutex) ;
        fprintf (stdout ,"[parent] sleep 5 sec .\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000)); // 10 sec
        memcpy(pdata->data, msg.c_str(),msg.size());
        pthread_mutex_unlock(shared_mutex);

        fprintf(stdout,"[parent] unlock mutex.\n");
    }
    else
    {
        fprintf(stdout,"[parent] cannot create or open shared mutex.errno = %d\n",shmem.lastErrno());
    }

    return 0;
}

int test_refshmemref()
{
    SharedMem<MyData> shmem;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    shmem = std::move(SharedMem<MyData>(SHARED_MEM_NAME));
    if (shmem.valid())
    {
        // clear
        MyData * pdata = shmem.reference();

        // 初始化互斥锁
        auto shared_mutex = &pdata->mutex;

        fprintf(stdout,"[child] prepare lock mutex.\n");
        pthread_mutex_lock(shared_mutex) ;
        fprintf(stdout,"[child] read memory content = %s .\n",pdata->data);
        pthread_mutex_unlock(shared_mutex);
        fprintf(stdout,"[child] unlock mutex.\n");
    }
    else
    {
        fprintf(stdout,"[child] cannot open shared mutex.errno = %d\n",shmem.lastErrno());
    }

    return 0;
}

int test_shmem()
{
    int pid = 0;
    if ((pid = fork()) == 0)
    {
        // child
        test_refshmemref();
    }
    else
    {
        test_shmemowner();
    }
    ::wait(0);
    return 0;
}

int main(int argc, char **argv)
{
    test_shmem();
    return 0;
}
