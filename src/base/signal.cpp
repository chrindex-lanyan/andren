
#include <signal.h>
#include <assert.h>
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
        int ret = unlink();
    }

    PosixNamedSignal::PosixNamedSignal(PosixNamedSignal &&_)
    {
        m_sem = _.m_sem;
        m_name = _.m_name;
        _.m_sem = 0;
        _.m_name.clear();
    }

    PosixNamedSignal &PosixNamedSignal::operator=(PosixNamedSignal &&_)
    {
        unlink();
        m_sem = _.m_sem;
        m_name = _.m_name;
        _.m_sem = 0;
        _.m_name.clear();
    }

    bool PosixNamedSignal ::open(const std::string &name, int flags,
                                 std::function<void(int errNO)> whenSEM_FAILED, unsigned int mode, unsigned int value)
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
        m_sem = ret;
        if (ret == SEM_FAILED)
        {
            if (whenSEM_FAILED)
            {
                whenSEM_FAILED(errno);
            }
            else
            {
                ::sem_unlink(name.c_str());
                m_sem = 0;
                return false;
            }
        }
        m_name = name;
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

    int PosixNamedSignal::typeno() const
    {
        return 1;
    }

    PosixUnnamedSignal::PosixUnnamedSignal(int value)
    {
        m_sem = new sem_t {};
        int ret = ::sem_init(m_sem, 0, value);
        if (ret!=0){
            delete m_sem;
            m_sem = 0;
        }
    }

    PosixUnnamedSignal::~PosixUnnamedSignal()
    {
        if(m_sem){
            ::sem_destroy(m_sem);
        }
        
    }

    PosixUnnamedSignal::PosixUnnamedSignal(PosixUnnamedSignal && _)
    {
        if (m_sem==0){
            m_sem = new sem_t{};
        }
        memcpy(m_sem,_.m_sem,sizeof(m_sem));
        memset(_.m_sem,0,sizeof(m_sem));
        delete _.m_sem;
        _.m_sem = 0;
    }

    PosixUnnamedSignal &PosixUnnamedSignal::operator=(PosixUnnamedSignal && _)
    {
        if (m_sem){
            ::sem_destroy(m_sem);
            delete m_sem;
        }
        
        memcpy(&m_sem,&_.m_sem,sizeof(m_sem));
        memset(&_.m_sem,0,sizeof(m_sem));
        delete _.m_sem;
        _.m_sem = 0;
    }

    sem_t *PosixUnnamedSignal::sem()
    {
        return m_sem;
    }

    int PosixUnnamedSignal::typeno() const
    {
        return 2;
    }

    PosixSignal::PosixSignal() { m_handle = 0; }

    PosixSignal::~PosixSignal() {}

    PosixSignal::PosixSignal(PosixSignal &&)
    {
    }

    PosixSignal &PosixSignal::operator=(PosixSignal &&)
    {
    }

    void PosixSignal::setHandle(PosixSigHandle *handle)
    {
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
        timespec tsp = {0, 0};
        int ret = ::clock_gettime(CLOCK_REALTIME, &tsp);
        assert(ret == 0);

        uint64_t nsec = (tsp.tv_sec * 1000 * 1000 * 1000) + (tsp.tv_nsec) + (msec * 1000 * 1000);

        tsp.tv_sec = nsec / (1000 * 1000 * 1000);
        tsp.tv_nsec = nsec % (1000 * 1000 * 1000);
        return ::sem_timedwait(m_handle->sem(), &tsp);
    }

    int PosixSignal::post()
    {
        return ::sem_post(m_handle->sem());
    }

    int PosixSignal::value(int &val)
    {
        return ::sem_getvalue(m_handle->sem(), &val);
    }

    bool PosixSignal::valid() const
    {
        return m_handle->sem() != 0 && m_handle->sem() != SEM_FAILED;
    }

    PosixSigHandle *PosixSignal::handle() const
    {
        return m_handle;
    }

}