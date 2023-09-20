
#pragma once

#include <stdint.h>

namespace chrindex ::andren::base
{

    /// @brief 获取一个单调自增的32位整形uid。
    /// thread-safe : true
    /// @return uid i32
    uint32_t create_uid_u32();

    /// @brief 获取一个单调自增的64位整形uid。
    /// thread-safe : true
    /// @return uid u32
    uint64_t create_uid_u64();

}


