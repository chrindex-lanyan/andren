#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <string>

#include "noncopyable.hpp"

namespace chrindex::andren::base
{

    struct ShmOwner
    {
    };
    using shm_owner = ShmOwner ;

    struct ShmReference
    {
    };
    using shm_borrow = ShmReference;

    template <typename T>
    class ShmMutex
    {
    public:
        bool valid() const
        {
            return false;
        }
    };
    template<typename T>
    using shm_mutex = ShmMutex<T>;

    template <>
    class ShmMutex<ShmOwner> : public noncopyable
    {
    public:
        using ShmMutexOwner = ShmMutex<ShmOwner>;

        ShmMutex() : m_shared_mutex(0) {}

        ShmMutex(std::string const &path) : m_shared_mutex(0)
        {
            // 先判断共享内存存不存在
            int shm_fd = shm_open(path.c_str(), O_RDWR, 0666);
            if (shm_fd == -1 && errno == ENOENT)
            {
                // 创建或打开共享内存对象
                shm_fd = shm_open(path.c_str(), O_CREAT | O_RDWR, 0666);
                if (shm_fd == -1)
                {
                    perror("shm_open");
                    return;
                }
            }

            // 调整共享内存对象的大小
            if (ftruncate(shm_fd, sizeof(pthread_mutex_t)) == -1)
            {
                perror("ftruncate");
                return;
            }

            // 将共享内存对象映射到进程的虚拟内存空间
            auto shared_mutex = (pthread_mutex_t *)mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shared_mutex == MAP_FAILED)
            {
                perror("mmap");
                return;
            }

            // 初始化互斥锁
            pthread_mutexattr_t mutex_attr;
            pthread_mutexattr_init(&mutex_attr);
            pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(shared_mutex, &mutex_attr);

            m_shared_mutex = shared_mutex;
            m_path = path;
        }

        ShmMutex(ShmMutexOwner &&_)
        {
            m_shared_mutex = _.m_shared_mutex;
            _.m_shared_mutex = 0;
            m_path = _.m_path;
            _.m_path.clear();
        }

        ~ShmMutex()
        {
            if (m_shared_mutex == 0)
            {
                return;
            }
            auto shared_mutex = m_shared_mutex;

            // 解除共享内存对象的映射
            if (munmap(shared_mutex, sizeof(pthread_mutex_t)) == -1)
            {
                perror("munmap");
                return;
            }

            // 删除共享内存对象
            if (shm_unlink(m_path.c_str()) == -1)
            {
                perror("shm_unlink");
                return;
            }
        }

        ShmMutexOwner &operator=(ShmMutexOwner &&_)
        {
            if (m_shared_mutex)
            {
                this->~ShmMutex();
            }
            m_shared_mutex = _.m_shared_mutex;
            _.m_shared_mutex = 0;
            m_path = _.m_path;
            _.m_path.clear();
            return *this;
        }

        int lock()
        {
            int ret = 0;
            if (m_shared_mutex)
            {
                ret = pthread_mutex_lock(m_shared_mutex);
            }
            else
            {
                return -1;
            }
            return ret;
        }

        int unlock()
        {
            int ret = 0;
            if (m_shared_mutex)
            {
                ret = pthread_mutex_unlock(m_shared_mutex);
            }
            else
            {
                return -1;
            }
            return ret;
        }

        template <typename Fn, typename... ARGS>
        bool scope_run(Fn &&fn, ARGS... args)
        {
            int ret = 0;
            if ((ret = lock()) == 0)
            {
                fn(std::forward<ARGS>(args)...);
                ret = unlock();
            }
            return ret;
        }

        bool valid() const
        {
            return m_shared_mutex != 0;
        }

        static int lastErrno()
        {
            return errno;
        }
        std::string path() const { return m_path; }

    private:
        pthread_mutex_t *m_shared_mutex;
        std::string m_path;
    };


    template <>
    class ShmMutex<ShmReference> : public noncopyable
    {
    public:
        using ShmMutexReference = ShmMutex<ShmReference>;

        ShmMutex() : m_shared_mutex(0) {}

        ShmMutex(std::string const &path) : m_shared_mutex(0)
        {
            // 先判断共享内存存不存在
            int shm_fd = shm_open(path.c_str(), O_RDWR, 0666);
            if (shm_fd == -1 && errno == ENOENT)
            {
                perror("shm_open");
                return;
            }

            // 将共享内存对象映射到进程的虚拟内存空间
            auto shared_mutex = (pthread_mutex_t *)mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shared_mutex == MAP_FAILED)
            {
                perror("mmap");
                return;
            }

            m_shared_mutex = shared_mutex;
            m_path = path;
        }

        ShmMutex(ShmMutexReference &&_)
        {
            m_shared_mutex = _.m_shared_mutex;
            _.m_shared_mutex = 0;
            m_path = _.m_path;
            _.m_path.clear();
        }

        ~ShmMutex()
        {
            if (m_shared_mutex == 0)
            {
                return;
            }
            auto shared_mutex = m_shared_mutex;

            // 解除共享内存对象的映射
            if (munmap(shared_mutex, sizeof(pthread_mutex_t)) == -1)
            {
                perror("munmap");
                return;
            }
        }

        ShmMutexReference &operator=(ShmMutexReference &&_)
        {
            if (m_shared_mutex)
            {
                this->~ShmMutex();
            }
            m_shared_mutex = _.m_shared_mutex;
            _.m_shared_mutex = 0;
            m_path = _.m_path;
            _.m_path.clear();
            return *this;
        }

        int lock()
        {
            int ret = 0;
            if (m_shared_mutex)
            {
                ret = pthread_mutex_lock(m_shared_mutex);
            }
            else
            {
                return -1;
            }
            return ret;
        }

        int unlock()
        {
            int ret = 0;
            if (m_shared_mutex)
            {
                ret = pthread_mutex_unlock(m_shared_mutex);
            }
            else
            {
                return -1;
            }
            return ret;
        }

        template <typename Fn, typename... ARGS>
        bool scope_run(Fn &&fn, ARGS... args)
        {
            int ret = 0;
            if ((ret = lock()) == 0)
            {
                fn(std::forward<ARGS>(args)...);
                ret = unlock();
            }
            return ret;
        }

        bool valid() const
        {
            return m_shared_mutex != 0;
        }

        static int lastErrno()
        {
            return errno;
        }
        std::string path() const { return m_path; }

    private:
        pthread_mutex_t *m_shared_mutex;
        std::string m_path;
    };

    template <typename T>
    using shm_mutex = ShmMutex<T>;

}