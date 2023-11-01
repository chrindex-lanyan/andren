

#include <pthread.h>
#include <functional>
#include <signal.h>

#include "thread.hh"

namespace chrindex ::andren::base
{

    Thread::Thread() : m_pthread(0), m_detach(0) {}

    Thread::Thread(Thread &&_thread)
    {
        m_pthread = _thread.m_pthread;
        m_detach = _thread.m_detach;
        _thread.m_pthread = 0;
        _thread.m_detach = 0;
    }

    Thread &Thread::operator=(Thread &&_thread)
    {
        if (m_pthread > 0)
        {
            throw "cannot drop a working thread "
                  "to move assignments for a working threads";
        }
        m_pthread = _thread.m_pthread;
        m_detach = _thread.m_detach;
        _thread.m_pthread = 0;
        _thread.m_detach = 0;
        
        return *this;
    }

    Thread::~Thread()
    {
        if (m_pthread > 0 && m_detach != 0)
        {
            join();
        }
    }

    int Thread::create(func_t func)
    {
        func_t *_pfunc = new decltype(func)(std::move(func));
        pthread_t ptfd = 0;
        int ret =
            pthread_create(&ptfd,
                           0,
                           &Thread::work,
                           _pfunc);

        if (ret == 0)
        {
            m_pthread = ptfd;
        }
        return ret;
    }

    int Thread::set_name(const std::string &name)
    {
        return pthread_setname_np(m_pthread, name.c_str());
    }

    std::string Thread::name() const
    {
        std::string name(128, 0);
        int ret =
            pthread_getname_np(m_pthread, &name[0], name.size());
        if (ret == 0)
        {
            return name;
        }
        return {};
    }

    int Thread::detach()
    {
        int ret = pthread_detach(m_pthread);
        m_detach = (ret == 0);
        return ret;
    }

    int Thread::join()
    {
        void *arg;
        return pthread_join(m_pthread, &arg);
    }

    int Thread::try_join()
    {
        void *arg;
        return pthread_tryjoin_np(m_pthread, &arg);
    }

    int Thread::time_join(int msec)
    {
        void *arg;
        timespec ts;
        ts.tv_nsec = (msec % 1000) * 1000;
        ts.tv_sec = msec / 1000;
        return pthread_timedjoin_np(m_pthread, &arg, &ts);
    }

    int Thread::kill(int sig)
    {
        return pthread_kill(m_pthread, sig);
    }

    int Thread::yield()
    {
        return sched_yield();
    }

    pthread_t Thread::self()
    {
        return pthread_self();
    }

    void *Thread::work(void *arg)
    {
        func_t func;
        if (arg != nullptr)
        {
            auto _pfunc =
                reinterpret_cast<decltype(func) *>(arg);
            func = std::move(*_pfunc);
            delete _pfunc;
        }
        if (func)
        {
            func();
        }
        return 0;
    }

    void Thread::pthread_self_exit()
    {
        void * arg = 0;
        ::pthread_exit(&arg);
    }

}