#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <string>

#include <type_traits>

#include "noncopyable.hpp"

namespace chrindex::andren::base
{

    template <typename T, typename = std::enable_if<std::is_trivial<T>::value && std::is_standard_layout<T>::value, T>::type>
    class SharedMem : public noncopyable
    {
    public:
        SharedMem() : m_pdata(0), m_owner(false) {}
        SharedMem(std::string const &path, bool owner = false) : m_pdata(0), m_owner(false)
        {
            int shm_fd = 0;
            if (owner)
            {
                // 先判断共享内存存不存在
                shm_fd = shm_open(path.c_str(), O_RDWR, 0666);
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
                if (ftruncate(shm_fd, sizeof(T)) == -1)
                {
                    perror("ftruncate");
                    return;
                }
            }
            else
            {
                // 先判断共享内存存不存在
                shm_fd = shm_open(path.c_str(), O_RDWR, 0666);
                if (shm_fd == -1 && errno == ENOENT)
                {
                    perror("shm_open");
                    return;
                }
            }

            // 将共享内存对象映射到进程的虚拟内存空间
            auto shared_data = (T *)mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shared_data == MAP_FAILED)
            {
                perror("mmap");
                return;
            }
            m_pdata = shared_data;
            m_owner = owner;
            m_path = path;
        }
        ~SharedMem()
        {
            close();
        }

        SharedMem(SharedMem &&_)
        {
            m_pdata = _.m_pdata;
            m_path = _.m_path;
            m_owner = _.m_owner;
            _.m_path.clear();
            _.m_pdata = 0;
            _.m_owner = false;
        }

        SharedMem &operator=(SharedMem &&_)
        {
            close();
            m_pdata = _.m_pdata;
            m_path = _.m_path;
            m_owner = _.m_owner;
            _.m_path.clear();
            _.m_pdata = 0;
            _.m_owner = false;
            return *this;
        }

        T *reference() { return m_pdata; }

        bool valid() const { return m_pdata != 0; }

        bool owner() const { return m_owner; }

        static int lastErrno() { return errno; }

        std::string path() const { return m_path; }

    private:
        void close()
        {
            if (m_pdata == 0)
            {
                return;
            }
            auto shared_data = m_pdata;

            // 解除共享内存对象的映射
            if (munmap(shared_data, sizeof(T)) == -1)
            {
                perror("munmap");
                return;
            }
            if (m_owner)
            {
                // 删除共享内存对象
                if (shm_unlink(m_path.c_str()) == -1)
                {
                    perror("shm_unlink");
                    return;
                }
            }
            m_pdata = 0;
            m_owner = false;
        }

    private:
        T *m_pdata;
        std::string m_path;
        bool m_owner;
    };

}
