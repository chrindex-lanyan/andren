#pragma once


#include <coroutine>
#include <utility>
#include <exception>

#include "memclear.hpp"

namespace chrindex::andren::base
{
    template <typename T>
    struct co_task;

    template<typename T>
    struct co_promise;

    template<typename T>
    class awaitable_template;


    template <typename T>
    struct co_task
    {
        using SelfType = co_task<T>;
        using promise_type = co_promise<T>;
        using corohandle = std::coroutine_handle<promise_type>;

        co_task() : m_handle(nullptr) {}

        co_task(corohandle han) : m_handle(han) {}

        co_task(co_task&& _) noexcept 
        { 
            m_handle = std::move(_.m_handle); 
            _.m_handle = nullptr;
        }

        template< typename FN>
        co_task(FN&& fn) 
        { 
            SelfType tmp = fn(); 
            m_handle = std::move(tmp.m_handle); 
            tmp.m_handle = nullptr;
        }

        template< typename FN, typename  ... ARGS>
        co_task(FN&& fn, ARGS ... args) 
        { 
            SelfType tmp = fn(std::forward<ARGS>(args)...); 
            m_handle = std::move(tmp.m_handle); 
            tmp.m_handle = nullptr;
        }

        void operator=(co_task&& _) noexcept
        { 
            this->~co_task();
            m_handle = std::move(_.m_handle);  
            _.m_handle = nullptr;
        }

        virtual ~co_task() { if (m_handle) { m_handle.destroy(); } }

        corohandle handle() { return m_handle; }

    protected:

        corohandle m_handle;
    };


    template<typename T>
    struct co_promise
    {
    public:

        co_promise() : m_value{} {}

        co_promise(T&& value) : m_value(std::move(value)) {   }

        co_promise(T const& value) :m_value(value) {}

        virtual ~co_promise() {}

    public:

        co_task<T> get_return_object() { return co_task<T>::corohandle::from_promise(*this); }

        void return_value(T&& v)
        {
            m_value = std::move(v);
            return;
        }

        void return_value(T const& v)
        {
            m_value = v;
            return;
        }

        std::suspend_always yield_value(T&& v)
        {
            m_value = std::move(v);
            return std::suspend_always{};
        }

        std::suspend_always yield_value(T const& v)
        {
            m_value = v;
            return std::suspend_always{};
        }
        
        struct unhandled_exception_t : public std::bad_exception { 
            unhandled_exception_t() noexcept {}
            const char* what() const noexcept  override { return "unhandled"; } };

        void unhandled_exception() { throw unhandled_exception_t{}; }

        std::suspend_always initial_suspend() { return std::suspend_always{}; };
        
        std::suspend_always final_suspend() noexcept { return std::suspend_always{}; };

    public :

        T m_value;
    };
   
 
    template<typename T>
    class awaitable_template
    {
    public:

        using awaitable_type = awaitable_template<T>;

        awaitable_template() {}

        awaitable_template(awaitable_type&& _) {}

        virtual ~awaitable_template() {}

        // 询问其是否就绪
        virtual bool await_ready() = 0;

        // 挂起时的操作。
        virtual void await_suspend(std::coroutine_handle<> handle) = 0;

        // 恢复时被调用。通常用于返回所需数据。
        virtual T await_resume() = 0;

    };
};
