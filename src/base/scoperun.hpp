#pragma once
namespace chrindex::andren::base
{

    #define SCOPE_RUN(LOCK_TYPE, mut, expr)       \
    do                                        \
    {                                         \
        LOCK_TYPE<decltype(mut)> locker(mut); \
        expr;                                 \
    } while (0)

}