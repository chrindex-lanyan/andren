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


    /// 装箱。用于提供number和指针的默认初始化
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>
    || std::is_same_v<double, T> || std::is_same_v<float, T> || std::is_pointer_v<T>>>
        struct InitialBox
    {
        InitialBox()
        {
            if constexpr (std::is_pointer_v<T>)
            {
                val = nullptr; // 初始化指针类型
            }
            else if  constexpr (std::is_integral_v<T>)
            {
                val = 0; // 初始化整数类型和char类型
            }
            else if constexpr (std::is_same_v<double, T> || std::is_same_v<float, T>)
            {
                val = 0.0f;
            }
            else 
            {
                val = 0;
            }
        }

        InitialBox(T v) :val(v) {}

        operator T& () { return val; }

        operator T const () const { return val; }

        T val;
    };


}