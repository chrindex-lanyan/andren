
#include "uid_creator.hh"

#include <atomic>
#include <thread>


namespace chrindex ::andren::base
{
    std::atomic<uint32_t> _uid_u32_last=1000;
    std::atomic<uint64_t> _uid_u64_last=1000;

    uint32_t create_uid_u32()
    {
        return _uid_u32_last.fetch_add(1, std::memory_order_seq_cst);
    }

    uint64_t create_uid_u64()
    {
        return _uid_u64_last.fetch_add(1, std::memory_order_seq_cst);
    }


}