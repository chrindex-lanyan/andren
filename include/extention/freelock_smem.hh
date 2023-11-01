#pragma once

#include "noncopyable.hpp"
#include "smem.hh"

#include <atomic>
#include <utility>
#include <string>
#include <stdint.h>

namespace chrindex::andren::network
{
    static inline constexpr auto SHMEM_SIZE_DEFAULT = 4096;
    

    /**
     * @brief single read single write SharedMemory.
     * Without Mutex Lock.
     */
    class FreeLockShareMemWriter : public base::noncopyable
    {
    public :

        FreeLockShareMemWriter(std::string const & path ,bool owner = false);
        FreeLockShareMemWriter(FreeLockShareMemWriter && _);
        ~FreeLockShareMemWriter();

        bool valid() const ;

        std::string path()const;

        bool writable() const;

        ssize_t write_some(std::string const & data);

        bool readend_closed()const ;

    private :

        struct alignas(4) ShDataPrivate_Real
        {
            std::atomic<int8_t> current_uid;
            std::atomic<int8_t> already_uid;
            uint16_t size;
            char data[SHMEM_SIZE_DEFAULT];
        };

        struct alignas(4) ShDataPrivate_POD
        {
            int8_t current_uid;
            int8_t already_uid;
            uint16_t size;
            char data[SHMEM_SIZE_DEFAULT];
        };

        union alignas(4) ShDataPrivate
        {
            ShDataPrivate_Real real;
            ShDataPrivate_POD pod;
        };

        base::SharedMem<ShDataPrivate_POD> m_shared_mem;
    };

    class FreeLockShareMemReader : public base::noncopyable
    {
    public :

        FreeLockShareMemReader(std::string const & path ,bool owner = false);
        FreeLockShareMemReader(FreeLockShareMemReader && _);
        ~FreeLockShareMemReader();

        bool valid() const ;

        std::string path()const;

        bool readable() const;

        ssize_t read_some(std::string & data);

        bool writend_closed() const ;

    private :

        struct alignas(4) ShDataPrivate_Real
        {
            std::atomic<int8_t> current_uid;
            std::atomic<int8_t> already_uid;
            uint16_t size;
            char data[SHMEM_SIZE_DEFAULT];
        };

        struct alignas(4) ShDataPrivate_POD
        {
            int8_t current_uid;
            int8_t already_uid;
            uint16_t size;
            char data[SHMEM_SIZE_DEFAULT];
        };

        union alignas(4) ShDataPrivate
        {
            ShDataPrivate_Real real;
            ShDataPrivate_POD pod;
        };

        base::SharedMem<ShDataPrivate_POD> m_shared_mem;
    };


};

