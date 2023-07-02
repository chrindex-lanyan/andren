#pragma once

#include <sys/types.h>

#include <stdint.h>
#include <vector>

namespace chrindex ::andren::base
{
    /// @brief 简单的Buffer类
    class buffer_t
    {
    public:
        buffer_t();

        buffer_t(const buffer_t &b);

        buffer_t(buffer_t &&_b) noexcept;

        buffer_t(const char *ptr, size_t size);

        template <typename T, size_t N>
        buffer_t(T (&array)[N])
        {
            m_data.resize(N);
            memcpy(&m_data[0], array, N);
        }

        buffer_t &operator=(const buffer_t &);

        buffer_t &operator=(buffer_t &&);

        explicit operator const char *() const noexcept;

        explicit operator bool() const noexcept;

        buffer_t &swap(buffer_t &_b) noexcept;

    public:
        buffer_t &add_data(const char *ptr, size_t size);

        template <typename T, size_t N>
        buffer_t &add_data(T (&array)[N])
        {
            m_data.reserve(m_data.size() + N);
            for (size_t i = 0; i < N; i++)
            {
                m_data.push_back(array[i]);
            }
            return *this;
        }

        /// @brief 重设大小且尽可能保留数据
        /// @param size
        void resize(size_t size);

        /// @brief 重设大小且清空数据
        /// @param size
        void reset(size_t size);

        /// @brief 清空所有数据
        void zero();

        /// @brief 清空指定大小的数据，不超过缓冲区大小。
        /// @param size
        void zero(size_t size);

        char &operator[](size_t index);

    private:
        std::vector<char> m_data;
    };
}