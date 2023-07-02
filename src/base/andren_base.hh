#pragma once


#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <type_traits>



namespace chrindex::andren::base
{
    /// @brief 清空POD对象内存
    /// @tparam T
    /// @tparam
    /// @param t
    template <typename T, class = std::enable_if<std::is_pod<T>::value>>
    inline void ZeroMemRef(T &t)
    {
        ::memset(&t, 0, sizeof(T));
    }

    /// @brief 清空缓冲区
    /// @param ptr
    /// @param size
    inline void ZeroMemPtr(void *ptr, size_t size)
    {
        ::memset(ptr, 0, size);
    }   

}
