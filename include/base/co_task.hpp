#pragma once


#include <coroutine>
#include <utility>
#include <exception>


namespace chrindex::andren::base
{
    template <typename T>
    struct co_task;

    template<typename T>
    struct co_promise;

    template<typename T>
    class co_awaitable;

    template<typename T>
    struct co_result;

    template<>
    struct co_result<void>
    {
        using co_result_t = co_result<void>;

        co_result() : except_ptr(nullptr) {}

        co_result(std::exception_ptr ptr) :  except_ptr(nullptr) {}

        co_result(co_result_t&& _) noexcept
        {
            except_ptr = std::move(_.except_ptr);
            _.except_ptr = nullptr;
        }

        co_result_t& operator=(co_result_t&& _) noexcept
        {
            except_ptr = std::move(_.except_ptr);
            _.except_ptr = nullptr;
            return *this;
        }

        std::exception_ptr except_ptr;
    };

    template<typename T>
    struct co_result 
    {
        using co_result_t = co_result<T>;

        co_result() :val{} , except_ptr(nullptr) {}

        co_result(T const& v) : val{ v }, except_ptr(nullptr) {}

        co_result(T && v) : val{ std::move(v) }, except_ptr(nullptr) {}

        co_result(std::exception_ptr ptr) : val{}, except_ptr(nullptr) {}

        co_result(co_result_t && _)  
        {
            val = std::move(_.val);
            except_ptr = std::move(_.except_ptr);
            _.except_ptr = nullptr;
        }

        co_result_t& operator=(co_result_t && _)
        {
            val = std::move(_.val);
            except_ptr = std::move(_.except_ptr);
            _.except_ptr = nullptr;
            return *this;
        }

        T val;
        std::exception_ptr except_ptr;
    };

    template <typename T>
    struct co_task
    {
        using co_task_t = co_task<T>;
        using promise_type = co_promise<T>;
        using co_handle = std::coroutine_handle<promise_type>;

        co_task() : m_handle(nullptr) {}

        co_task(co_handle handle) : m_handle(handle) {}

        co_task(co_task&& _) noexcept
        {
            m_handle = std::move(_.m_handle);
            _.m_handle = nullptr;
        }

        template< typename FN>
        co_task(FN&& fn)
        {
            co_task_t tmp = fn();
            m_handle = std::move(tmp.m_handle);
            tmp.m_handle = nullptr;
        }

        template< typename FN, typename  ... ARGS>
        co_task(FN&& fn, ARGS ... args)
        {
            co_task_t tmp = fn(std::forward<ARGS>(args)...);
            m_handle = std::move(tmp.m_handle);
            tmp.m_handle = nullptr;
        }

        void operator=(co_task_t && _) noexcept
        {
            this->~co_task();
            m_handle = std::move(_.m_handle);
            _.m_handle = nullptr;
        }

        virtual ~co_task() { if (m_handle) { m_handle.destroy(); } }

        co_handle handle() { return m_handle; }

    protected:

        co_handle m_handle;
    };


    template<>
    struct co_promise<void>
    {
    public:

        co_promise() {}

        virtual ~co_promise() {}

    public:

        co_task<void> get_return_object() { return co_task<void>::co_handle::from_promise(*this); }

        void return_void() {}

        std::suspend_always yield_value(int ) { return std::suspend_always{}; }

        struct unhandled_exception_t : public std::bad_exception {
            unhandled_exception_t() noexcept {}
            const char* what() const noexcept  override { return "unhandled"; }
        };

        void unhandled_exception() { throw unhandled_exception_t{}; }

        std::suspend_always initial_suspend() { return std::suspend_always{}; };

        std::suspend_always final_suspend() noexcept { return std::suspend_always{}; };

    };


    template<typename T>
    struct co_promise
    {
    public:

        co_promise() : m_value{} {}

        co_promise(T && value) : m_value(std::move(value)) {   }

        co_promise(T const& value) :m_value(value) {}

        virtual ~co_promise() {}

    public:

        co_task<T> get_return_object() { return co_task<T>::co_handle::from_promise(*this); }

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
            const char* what() const noexcept  override { return "unhandled"; }
        };

        void unhandled_exception() { throw unhandled_exception_t{}; }

        std::suspend_always initial_suspend() { return std::suspend_always{}; };

        std::suspend_always final_suspend() noexcept { return std::suspend_always{}; };

    public:

        T m_value;
    };


    template<>
    class co_awaitable<void>
    {
    public:

        using co_awaitable_t = co_awaitable<void>;

        co_awaitable() {}

        co_awaitable(co_awaitable_t&&) = default;

        virtual ~co_awaitable() {}

        // 询问其是否就绪
        virtual bool await_ready() = 0;

        // 挂起时的操作。
        virtual void await_suspend(std::coroutine_handle<> handle) = 0;

        // 恢复时被调用。通常用于返回所需数据。
        virtual void await_resume() = 0;

    };


    template<typename T>
    class co_awaitable
    {
    public:

        using co_awaitable_t = co_awaitable<T>;

        co_awaitable() {}

        co_awaitable(co_awaitable_t && _) {}

        virtual ~co_awaitable() {}

        // 询问其是否就绪
        virtual bool await_ready() = 0;

        // 挂起时的操作。
        virtual void await_suspend(std::coroutine_handle<> handle) = 0;

        // 恢复时被调用。通常用于返回所需数据。
        virtual T await_resume() = 0;

    };
};
