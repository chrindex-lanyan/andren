
#include <string>
#include <stdint.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

#include <stdio.h>

#include "mutex_que.hpp"


inline int64_t msectime() 
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


using namespace chrindex::andren::base;

size_t count = 1000 * 1000 * 10;

int testmutex_queue()
{
    mutex_que<int> mutex_queue;
    size_t count = ::count;

    fprintf(stdout, "mutex_queue : Test Type is `int` , size = %lu.\n", count);

    auto wrfn = [&mutex_queue, count]() mutable
    {
        int64_t msec = msectime();

        for (int i = 0; i < count; i++)
        {
            mutex_queue.pushBack(std::move(i + 1));
        }
        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Writer Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    auto rdfn = [&mutex_queue, count]() mutable
    {
        int64_t tmp = 0;
        size_t need = 0;
        std::optional<int> opt;

        int64_t msec = msectime();

        while (need < count)
        {
            opt.reset();
            mutex_queue.takeOne(opt);
            if (opt.has_value())
            {
                tmp += opt.value();
                need++;
            }
        }

        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Count = `%lu`, ALL = `%ld`.\n", need, tmp);
        fprintf(stdout, "Reader Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    std::thread wrthread(std::move(wrfn));
    std::thread rdthread(std::move(rdfn));

    wrthread.join();
    printf("write thread exit...\n");
    rdthread.join();
    printf("read thread exit...\n");

    fprintf(stdout, "Test mutex_queue Done\n");

    return 0;
}

int testmutex_queueMulti()
{
    size_t count = ::count;
    mutex_que<int> mutex_queue;

    fprintf(stdout, "\nmutex_queue Multi Get : Test Type is `int` , size = %lu.\n", count);

    auto wrfn = [&mutex_queue, count]() mutable
    {
        int64_t msec = msectime();

        for (int i = 0; i < count; i++)
        {
            mutex_queue.pushBack(std::move(i + 1));
        }
        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Writer Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    auto rdfn = [&mutex_queue, count]() mutable
    {
        int64_t tmp = 0;
        size_t need = 0;
        std::deque<int> res;
        int64_t msec = msectime();

        while (need < count)
        {
            mutex_queue.takeMulti(res);
            if (res.empty())
            {
                continue;
            }
            need += (int)res.size();
            for (auto& r : res)
            {
                tmp += r;
            }
        }

        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Count = `%lu`, ALL = `%ld`.\n", need, tmp);
        fprintf(stdout, "Reader Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    std::thread wrthread(std::move(wrfn));
    std::thread rdthread(std::move(rdfn));

    wrthread.join();
    printf("write thread exit...\n");
    rdthread.join();
    printf("read thread exit...\n");

    fprintf(stdout, "Test mutex_queue Done.\n");

    return 0;
}

int testmutex_queueCustomAlloc()
{
    size_t count = ::count;
    std::vector<uint64_t> buffer; // 8bytes
    buffer.resize(1024 * 1024 ); // 8 MB 
    std::pmr::monotonic_buffer_resource myalloc(&buffer[0],buffer.size());
    mutex_que_pmr<int, std::pmr::monotonic_buffer_resource> mutex_queue(&myalloc);

    fprintf(stdout, "\nmutex_queue With monotonic_buffer_resource : Test Type is `int` , size = %lu.\n", count);

    auto wrfn = [&mutex_queue, count]() mutable
    {
        int64_t msec = msectime();

        for (int i = 0; i < count; i++)
        {
            mutex_queue.pushBack(std::move(i + 1));
        }
        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Writer Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    auto rdfn = [&mutex_queue, count]() mutable
    {
        int64_t tmp = 0;
        size_t need = 0;
        std::optional<int> opt;
        int64_t msec = msectime();

        while (need < count)
        {
            opt.reset();
            mutex_queue.takeOne(opt);
            if (opt.has_value())
            {
                tmp += opt.value();
                need++;
            }
        }

        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Count = `%lu`, ALL = `%ld`.\n", need, tmp);
        fprintf(stdout, "Reader Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    std::thread wrthread(std::move(wrfn));
    std::thread rdthread(std::move(rdfn));

    wrthread.join();
    printf("write thread exit...\n");
    rdthread.join();
    printf("read thread exit...\n");

    fprintf(stdout, "Test mutex_queue With monotonic_buffer_resource Done.\n");

    buffer = {};

    return 0;
}

int testmutex_queueCustomAllocAndMultiGET()
{
    size_t count = ::count;
    std::vector<uint64_t> buffer; // 8bytes
    buffer.resize(1024 * 1024 ); // 8 MB
    std::pmr::monotonic_buffer_resource myalloc(&buffer[0], buffer.size());
    mutex_que_pmr<int, std::pmr::monotonic_buffer_resource> mutex_queue(&myalloc);

    fprintf(stdout, "\nmutex_queue MultiGet With monotonic_buffer_resource : Test Type is `int` , size = %lu.\n", count);

    auto wrfn = [&mutex_queue, count]() mutable
    {
        int64_t msec = msectime();

        for (int i = 0; i < count; i++)
        {
            mutex_queue.pushBack(std::move(i + 1));
        }
        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Writer Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    auto rdfn = [&mutex_queue, count, &myalloc]() mutable
    {
        int64_t tmp = 0;
        size_t need = 0;
        std::pmr::deque<int> res(&myalloc);
        int64_t msec = msectime();

        while (need < count)
        {
            mutex_queue.takeMulti(res);
            if (res.empty())
            {
                continue;
            }
            need += (int)res.size();
            for (auto& r : res)
            {
                tmp += r;
            }
        }

        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Count = `%lu`, ALL = `%ld`.\n", need, tmp);
        fprintf(stdout, "Reader Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    std::thread wrthread(std::move(wrfn));
    std::thread rdthread(std::move(rdfn));

    wrthread.join();
    printf("write thread exit...\n");
    rdthread.join();
    printf("read thread exit...\n");

    buffer = {};

    fprintf(stdout, "Test mutex_queue MultiGet With monotonic_buffer_resource Done.\n");

    return 0;
}

int test_circle_mutex_queue()
{
    size_t count = ::count;
    mutex_circle_que<int> mutex_queue;

    fprintf(stdout, "\nmutex_queue Circle : Test Type is `int` , size = %lu.\n", count);

    auto wrfn = [&mutex_queue, count]() mutable
    {
        int64_t msec = msectime();

        for (int i = 0; i < count; i++)
        {
            mutex_queue.pushBack(std::move(i + 1));
        }
        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Writer Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    auto rdfn = [&mutex_queue, count]() mutable
    {
        int64_t tmp = 0;
        size_t need = 0;
        std::optional<int> opt;
        int64_t msec = msectime();

        while (need < count)
        {
            opt.reset();
            mutex_queue.takeOne(opt);
            if (opt.has_value())
            {
                tmp += opt.value();
                need++;
            }
        }

        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Count = `%lu`, ALL = `%ld`.\n", need, tmp);
        fprintf(stdout, "Reader Thread Used Time %ld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    std::thread wrthread(std::move(wrfn));
    std::thread rdthread(std::move(rdfn));

    wrthread.join();
    printf("write thread exit...\n");
    rdthread.join();
    printf("read thread exit...\n");

    fprintf(stdout, "Test mutex_queue Circle Done.\n");

    return 0;
}


int main(int argc, char** argv) 
{
    count = 1000 * 1000 * 10; /// 1000 0 万
    testmutex_queue();
    testmutex_queueMulti();
    testmutex_queueCustomAlloc();
    testmutex_queueCustomAllocAndMultiGET();
    test_circle_mutex_queue();

    fprintf(stdout,"Test Done\n");
    return 0;
}





