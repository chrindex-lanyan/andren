

#include <functional>
#include <stdio.h>

#include "binaryheap.hpp"
#include "minheap.hpp"


using namespace chrindex::andren::base;


int test2x()
{
    fprintf(stdout, "Start Test 2xHeap!\n");

    MinHeap<BinaryHeap<uint64_t, std::function<void()>>> bhp;

    std::vector<int> testKey = {
        34, 12, 45, 6, 18, 27, 38, 10, 21, 32,
        41, 19, 28, 9, 15, 26, 37, 49, 22, 33,
        47, 14, 29, 40, 8, 17, 31, 43, 4, 11,
        25, 36, 48, 16, 30, 42, 7, 13, 24, 35,
        46, 5, 20, 39, 1, 23, 44, 2, 3, 50
    };

    for (auto& val : testKey) {
        bhp.push(val, [val]() {
            fprintf(stdout, "OK , This Key = %d .\n", val);
            });
    }

    while (bhp.size() > 0) {
        auto opt_pair = bhp.extract_min_pair();
        if (opt_pair.has_value()) {
            opt_pair.value().value()();
        }
    }

    fprintf(stdout, "Test 2xHeap Done!\n");

    return 0;
}

int main(int argc ,char ** argv){
    test2x();
    return 0;
}