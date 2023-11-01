


#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <fmt/core.h>
#include <iterator>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "freelock_smem.hh"


using namespace chrindex::andren;

std::string shmem_name = "/my_shared_memory";

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit , m_start;

int64_t now_msec ()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock().now().time_since_epoch()).count();
}


int  test_flshmem_ref()
{
    std::deque<std::string> result_que;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    network::FreeLockShareMemReader reader(shmem_name);

    std::string data;
    ssize_t size = 0;

    assert(reader.valid());

    while(m_exit == 0)
    {
        if (reader.readable())
        {
            size = reader.read_some(data);
            //genout("Child :: Read Some Data(%ld bytes) :%s.\n",size,data.c_str());
            result_que.push_back(fmt::format("Child :: Read Some Data({} bytes) :{}.\n", size, data));
        }

        if (result_que.size() > 100 || m_exit == 1)
        {
            std::thread([results = std::move(result_que)]()
            {
                for (auto & r: results)
                {
                    ::fwrite(r.c_str(),1,r.size(),stdout);
                }
            }).detach();
        }
        else 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    genout("Child : Exit...");

    return 0;
}


int test_flshmem_own()
{
    network::FreeLockShareMemWriter writer(shmem_name, true);
    int count = 1;

    assert(writer.valid());

    while(m_exit == 0)
    {
        if(writer.writable())
        {
            writer.write_some(fmt::format("Hello World -- {}.", count));
            count++;
        }
        else 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return 0;
}


int benchmark_write(int size, int count)
{
    std::string data(size, 0xff);
    network::FreeLockShareMemWriter writer(shmem_name, true);

    assert(writer.valid());

    while(m_start != 1)
    {
        std::this_thread::yield();
    }
    genout("Parent :: FL SharedMemory : Starting Write Data ... .\n");
    
    int64_t msec = now_msec();
    int need = count;

    while(m_exit == 0 && need)
    {
        if(writer.writable())
        {
            writer.write_some(data);
            need--;
        }
        std::this_thread::yield();
    }

    msec = now_msec() - msec;
    double speed = ((1.0 * size * count) / (1024 * 1024)) 
                    / ( 1.0 * msec / 1000) ;

    genout("Parent :: Write Speed :: %.2lf MB/S.\n",  speed);

    return 0;
}

int benchmark_read(int size, pid_t parent)
{
    ::kill(parent,SIGUSR1);
    genout("Child :: FL SharedMemory : Starting Read Data ... .\n");

    network::FreeLockShareMemReader reader(shmem_name);

    std::string data;
    size_t count = 0;
    ssize_t ret =0;
    size_t allsize = 0;

    assert(reader.valid());

    while(m_exit == 0)
    {
        if (reader.readable())
        {
            ret = reader.read_some(data);
            assert(ret == size);
            count ++;
            allsize += ret;
        }
        else 
        {
            std::this_thread::yield();
        }
    }

    genout("Child :: Read Count = %lu, Per Size = %lu , Exit....\n",count , (allsize/count));

    return 0;
}


int test_freelockshmem(int argc, char **argv)
{
    m_exit = 0;
    m_start = 0;
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   genout(" \n::准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        errout("Cannot registering signal handler");
        return -3;
    }

    int pid = 0;
    pid_t parent_pid = ::getpid();
    if ((pid = fork()) == 0)
    {
        // child
        if (argc >1)
        {
            benchmark_read(std::stoi(argv[1]),parent_pid);
        }
        else{
            test_flshmem_ref();
        }
    }
    else
    {
        if (argc > 1)
        {
            if (signal(SIGUSR1,
               [](int sig) -> void
               {
                   m_start = 1;
               }) == SIG_ERR)
            {
                errout("Cannot registering signal handler");
                return -3;
            }

            benchmark_write(std::stoi(argv[1]), std::stoi(argv[2]));
            ::kill(pid,SIGINT);
        }
        else 
        {
            test_flshmem_own();
        }
    }
    ::wait(0);
    return 0;
}

int main(int argc, char **argv)
{
    test_freelockshmem(argc, argv);
    return 0;
}
