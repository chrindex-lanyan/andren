
#include <signal.h>

#include <errno.h>
#include <time.h>

#include <string.h>

#include "signal.hh"

namespace chrindex::andren::base
{

    PosixNamedSignal::PosixNamedSignal()
    {
        m_sem = 0;
    }

    PosixNamedSignal::~PosixNamedSignal()
    {
        unlink();
    }

    bool PosixNamedSignal ::open(const std::string &name,
                                 int flags, unsigned int mode, unsigned int value)
    {
        sem_t *ret = 0;
        if (mode == (~0) || value == (~0))
        {
            ret = ::sem_open(name.c_str(), flags);
        }
        else
        {
            ret = ::sem_open(name.c_str(), flags, mode, value);
        }

        if (ret == 0)
        {
            ::sem_unlink(name.c_str());
            return false;
        }
        m_sem = ret;
        return true;
    }

    void PosixNamedSignal::close()
    {
        if (m_sem != 0)
        {
            ::sem_close(m_sem);
        }
    }

    int PosixNamedSignal::clockWait(unsigned int msec)
    {
        if (m_sem != 0)
        {
            timespec tsp = {0, 0};
            ::clock_gettime(CLOCK_MONOTONIC, &tsp); // 获取单调时钟时间，该时间不随系统时间变化而变化。
            tsp.tv_sec += msec / 1000;
            tsp.tv_nsec += (msec % 1000) * 1000 * 1000; // 毫->微->纳
            int ret = ::sem_clockwait(m_sem, CLOCK_MONOTONIC, &tsp);
            return ret;
        }
        return -1;
    }

    int PosixNamedSignal::unlink()
    {
        if (m_sem)
        {
            return ::sem_unlink(m_name.c_str());
        }
        return -1;
    }

    sem_t *PosixNamedSignal::sem()
    {
        return m_sem;
    }

    std::string PosixNamedSignal::name() const
    {
        return m_name;
    }

    int PosixNamedSignal::typeno()const {
        return 1;
    }

    PosixUnnamedSignal::PosixUnnamedSignal(int value)
    {
        int ret = ::sem_init(&m_sem, 0, value);
    }

    PosixUnnamedSignal::~PosixUnnamedSignal()
    {
        ::sem_destroy(&m_sem);
    }

    sem_t *PosixUnnamedSignal::sem()
    {
        return &m_sem;
    }

    int PosixUnnamedSignal::typeno()const {
        return 2;
    }

    PosixSignal::PosixSignal() { m_handle = 0; }

    PosixSignal::~PosixSignal() {}

    void PosixSignal::setHandle(PosixSigHandle * handle){
        m_handle = handle;
    }

    int PosixSignal::wait()
    {
        return ::sem_wait(m_handle->sem());
    }

    int PosixSignal::tryWait()
    {
        return ::sem_trywait(m_handle->sem());
    }

    int PosixSignal::timeWait(int msec)
    {
        timespec tsp={0,0};
        ::clock_gettime(CLOCK_MONOTONIC,&tsp);
        tsp.tv_sec += msec / 1000;
        tsp.tv_nsec += (msec %1000) * 1000 * 1000;
        return ::sem_timedwait(m_handle->sem(), &tsp);
    }

    int PosixSignal::post()
    {
        return ::sem_post(m_handle->sem());
    }

    int PosixSignal::value(int &val)
    {
        return ::sem_getvalue(m_handle->sem(),&val);
    }

    bool PosixSignal::valid() const
    {
        return m_handle->sem() !=0;
    }

}