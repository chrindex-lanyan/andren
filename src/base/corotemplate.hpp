#pragma once


#include <coroutine>
#include <utility>
#include <exception>

namespace chrindex::andren::base
{
    template <typename T>
    struct coro_base;

    template<typename T>
    struct promise_base;

    template<typename T>
    class awaitable_template;


    template <typename T>
    struct coro_base
    {
        using SelfType = coro_base<T>;
        using promise_type = promise_base<T>;
        using corohandle = std::coroutine_handle<promise_type>;

        coro_base() : m_handle(nullptr) {}

        coro_base(corohandle han) : m_handle(han) {}

        coro_base(coro_base&& _) noexcept 
        { 
            m_handle = std::move(_.m_handle); 
            _.m_handle = nullptr;
        }

        template< typename FN, typename  ... ARGS>
        coro_base(FN&& fn, ARGS ... args) 
        { 
            SelfType tmp = fn(std::forward<ARGS>(args)...); 
            m_handle = std::move(tmp.m_handle); 
            tmp.m_handle = nullptr;
        }

        void operator=(coro_base&& _) noexcept
        { 
            this->~coro_base();
            m_handle = std::move(_.m_handle);  
            _.m_handle = nullptr;
        }

        virtual ~coro_base() { if (m_handle) { m_handle.destroy(); } }

        corohandle handle() { return m_handle; }

    protected:

        corohandle m_handle;
    };


    template<typename T>
    struct promise_base
    {
    public:

        promise_base() : m_value{} {}

        promise_base(T&& value) : m_value(std::move(value)) {   }

        promise_base(T const& value) :m_value(value) {}

        virtual ~promise_base() {}

    public:

        coro_base<T> get_return_object() { return coro_base<T>::corohandle::from_promise(*this); }

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

        virtual void unhandled_exception() { throw unhandled_exception_t{}; }

        virtual std::suspend_always initial_suspend() { return std::suspend_always{}; };
        
        virtual std::suspend_always final_suspend() noexcept { return std::suspend_always{}; };

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
        virtual bool await_ready() const = 0;

        // 挂起时的操作。
        virtual void await_suspend(std::coroutine_handle<> handle) = 0;

        // 恢复时被调用。通常用于返回所需数据。
        virtual T await_resume() = 0;

    };
};
