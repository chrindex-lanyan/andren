﻿#pragma once

namespace chrindex::andren::base
{
    /// @brief 不许复制
    class noncopyable
    {
    public:
        noncopyable(const noncopyable &) = delete;
        void operator=(const noncopyable &) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
}