


#include <string>
#include <stdint.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

#include <stdio.h>

#include "../include/andren.hh"


inline int64_t msectime() 
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


using namespace chrindex::andren::base;


int testDBuffer()
{
    DBuffer<int> dbuffer;
    int count = 1000 * 1000 * 10;

    fprintf(stdout, "DBuffer : Test Type is `int` , size = %d.\n", count);

    auto wrfn = [&dbuffer, count]() mutable
    {
        int64_t msec = msectime();

        for (int i = 0; i < count; i++)
        {
            dbuffer.pushBack(std::move(i + 1));
        }
        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Writer Thread Used Time %lld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    auto rdfn = [&dbuffer, count]() mutable
    {
        int64_t tmp = 0;
        int need = 0;
        std::optional<int> opt;

        int64_t msec = msectime();

        while (need < count)
        {
            opt.reset();
            dbuffer.takeOne(opt);
            if (opt.has_value())
            {
                tmp += opt.value();
                need++;
            }
        }

        msec = msectime() - msec;
        double speed = ((1.0 * (count * sizeof(int32_t)) / msec) * 1000) / (1024 * 1024);
        fprintf(stdout, "Count = `%d`, ALL = `%lld`.\n", need, tmp);
        fprintf(stdout, "Reader Thread Used Time %lld Msec.Speed = %lf MB/s.\n", msec, speed);
    };

    std::thread wrthread(std::move(wrfn));
    std::thread rdthread(std::move(rdfn));

    wrthread.join();
    printf("write thread exit...\n");
    rdthread.join();
    printf("read thread exit...\n");

    fprintf(stdout, "Test DBuffer Done\n");

    return 0;
}



int main(int argc, char** argv) 
{
    testDBuffer();

    fprintf(stdout,"Test Done\n");
    return 0;
}





