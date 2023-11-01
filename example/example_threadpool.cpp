
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include "threadpool.hh"
#include "thread.hh"

using namespace chrindex::andren::base;


int test_threadpool()
{
    uint32_t thread_count = hardware_vcore_count();    
    ThreadPool tpool = ThreadPool(thread_count);

    fprintf (stdout,"Try Create Thread (Count = %d) For ThreadPool.\n",thread_count);

    for (uint32_t i = 0; i< thread_count ; i++)
    {
        for (uint32_t all = 0 ; all < 100 ; all++)
        {
            bool bret = tpool.exec([i , all](){
                fprintf (stdout,"thread-no = %u , task-no = %u.\n",i,all);
            },i);
            assert(bret);
        }
    }

    Thread::pthread_self_exit();

    return 0;
}




int main(int argc, char ** argv)
{
    test_threadpool();
    return 0;
}