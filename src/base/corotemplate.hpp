#pragma once


#include <coroutine>
#include <functional>
#include <type_traits>
#include <utility>
#include <exception>
#include <optional>
#include <deque>
#include <tuple>

namespace chrindex::andren::base
{
    template <typename T>
    struct coroutine_template;

    template<typename T>
    struct promise_template;

    template<typename T>
    class awaitable_template;

    template<typename T>
    using coroutine_result = std::tuple<std::optional<T>,std::exception_ptr> ;

    template <typename T>
    struct coroutine_template
    {
        using coroutine_template_type = coroutine_template<T>;
        using promise_type = promise_template<T>;
        using coroutine_handle = std::coroutine_handle<promise_type>;

        coroutine_template() : m_handle(nullptr) {}
 
        coroutine_template(coroutine_handle handle) : m_handle(handle) {}

        coroutine_template(coroutine_template&& _c) noexcept 
        { 
            m_handle = std::move(_c.m_handle); 
            _c.m_handle = nullptr;
        }

        template< typename FN>
        coroutine_template(FN && fn) 
        { 
            coroutine_template_type tmp = fn(); 
            m_handle = std::move(tmp.m_handle); 
            tmp.m_handle = nullptr;
        }

        template< typename FN, typename  ... ARGS>
        coroutine_template(FN&& fn, ARGS ... args) 
        { 
            coroutine_template_type tmp = fn(std::forward<ARGS>(args)...); 
            m_handle = std::move(tmp.m_handle); 
            tmp.m_handle = nullptr;
        }

        void operator=(coroutine_template&& _) noexcept
        { 
            this->~coroutine_template();
            m_handle = std::move(_.m_handle);  
            _.m_handle = nullptr;
        }

        ~coroutine_template() { if (m_handle) { m_handle.destroy(); } }

        auto and_then(std::function<void ()> && func)
        {
            m_handle.promise()
                .on_completed([func = std::forward<std::function<void()>>(func)]
                (T & result) -> coroutine_result<T>
                {
                    try {
                        func(result().result());
                    } catch (std::exception &e) {
                        // 忽略异常
                    }
                });
            return *this;
        }

        void finally(std::function<void ()> && func)
        {
            and_then(std::forward<std::function<void ()>>(func));
        }

        coroutine_handle handle() { return m_handle; }

        auto & result(){ return m_handle.promise().result(); }

        auto const & result_const() const { return m_handle.promise().result_const(); }

    private:
        coroutine_handle m_handle;
    };


    template<typename T>
    struct promise_template
    {
    public:

        using coroutine_callback = std::function<void((coroutine_result<T> & ))>;

        promise_template() : m_coro_result_opt{} {}

        promise_template(T&& value) : m_coro_result_opt({std::move(value),{nullptr}}) {   }

        promise_template(T const& value) :m_coro_result_opt({value,{nullptr}}) {}

        ~promise_template() {}

    public:

        auto & result ()
        { 
            auto &res_opt = std::get<0>(m_coro_result_opt);
            auto &exception_ptr = std::get<1>(m_coro_result_opt);
            if (!res_opt.has_value())
            {
                std::rethrow_exception(exception_ptr);
            }
            return res_opt.value();
        }

        auto const & result_const () const 
        { 
            auto &res_opt = std::get<0>(m_coro_result_opt);
            auto &exception_ptr = std::get<1>(m_coro_result_opt);
            if (!res_opt.has_value())
            {
                std::rethrow_exception(exception_ptr);
            }
            return res_opt.value();
        }

        coroutine_template<T> get_return_object() 
        { 
            return std::coroutine_handle<promise_template<T>>::from_promise(*this); 
        }

        auto await_transform(coroutine_template<T> &&coro_task) 
        {
            return awaitable_template<T>(std::move(coro_task));
        }

        void return_value(T&& v)
        {
            auto &res_opt = std::get<0>(m_coro_result_opt);
            res_opt = std::move(v);
            processCallbacks();
            return;
        }

        void return_value(T const& v)
        {
            auto &res_opt = std::get<0>(m_coro_result_opt);
            res_opt = v;
            processCallbacks();
            return;
        }

        auto yield_value(T&& v)
        {
            auto &res_opt = std::get<0>(m_coro_result_opt);
            res_opt = std::move(v);
            return std::suspend_always{};
        }

        auto yield_value(T const& v)
        {
            auto &res_opt = std::get<0>(m_coro_result_opt);
            res_opt = v;
            return std::suspend_always{};
        }
        
        struct unhandled_exception_t : public std::bad_exception 
        { 
            unhandled_exception_t() noexcept {}
            const char* what() const noexcept  override { return "unhandled"; } 
        };

        void unhandled_exception() 
        { 
            processCallbacks();
            throw unhandled_exception_t{}; 
        }

        auto initial_suspend() { return std::suspend_never{}; };
        
        auto final_suspend() noexcept { return std::suspend_always{}; };

        void on_complated(coroutine_callback &&func)
        {
            auto &res_opt = std::get<0>(m_coro_result_opt);
            if (res_opt.has_value())
            { 
                func(res_opt.value()); 
            }
            else 
            { 
                m_callbacks.push_back(std::forward<coroutine_callback>(func)); 
            }
        }

        void processCallbacks()
        {
            auto tmp = std::move(m_callbacks);
            for (auto & cb : tmp)
            {
                cb (m_coro_result_opt);
            }
        }

    private :
        coroutine_result<T> m_coro_result_opt;
        std::deque<coroutine_callback> m_callbacks;
    };
   
 
    template<typename T>
    class awaitable_template
    {
    public:

        using awaitable_type = awaitable_template<T>;

        awaitable_template() {}

        explicit awaitable_template(coroutine_template<T> &&coro_task) noexcept
            : m_coro_task(std::move(coro_task)) {}

        awaitable_template(awaitable_type&& _) :m_coro_task(std::move(_.m_coro_task)){}

        void operator=(awaitable_template<T> && _a){ m_coro_task = std::move(_a.m_coro_task); }

        awaitable_template(awaitable_template<T> const &) =delete;

        void operator=(awaitable_template<T> const & )=delete;

        ~awaitable_template() {}

        // 询问其是否就绪
        bool await_ready() { return false;}
            
        // 挂起时的操作。
        void await_suspend(std::coroutine_handle<> handle) 
        {
            m_coro_task.finally([handle]{
                handle.resume();
            });
        }

        // 恢复时被调用。通常用于返回所需数据。
        T await_resume() 
        { 
            if (m_coro_task.result().has_value())
            {
                return m_coro_task.result().value();
            }
            return {};
        }

    private :
        coroutine_template<T> m_coro_task;
    };
};
