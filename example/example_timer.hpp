#pragma once



#include "timermanager.hpp"

#include "binaryheap.hpp"
#include "minheap.hpp"
#include "KVPair.hpp"

#include <stdio.h>
#include <chrono>
#include <thread>

#include "example_test_macro.hh" 

using namespace chrindex::andren::base;


int test_timer()  {
    fprintf(stdout, "Start Test Timer Done!\n"); 

    TimerManger<MinHeap<BinaryHeap<uint64_t, KVPair<uint64_t,std::function<bool()>>>>> timerManager;

    /// 设置获取时间的函数
    timerManager.setTimeGetter([] (){
        return std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now().time_since_epoch()).count();
        });

    /// 一次性任务
    for (int i = 0; i < 100;i++) {
        auto v = i * 10 + 1;
        timerManager.create_task( v, [v]() ->bool{
            fprintf(stdout,"[TASK]::[%d]::[一次性任务] Done!!\n",v);
            return false;
        });
    }

    /// 循环执行的任务
    for (int i = 0; i < 50; i++) {
        timerManager.create_task(3000, [i]() ->bool {
            fprintf(stdout, "[TASK]::[%d]::[循环任务] Done!!\n", i);
            return true;
         });
    }

    /// 模拟事件循环检查和执行任务
    for (int i = 0 ;i < 30 * 1000;i++) // 50S
    {
        auto waitList = timerManager.check();
        for (auto &task : waitList) {
            timerManager.proxy_exec(task); // 代理执行会自动处理返回值为true的任务
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    fprintf(stdout ,"Test Timer Done!\n");

    return 0;
} 





