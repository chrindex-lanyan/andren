

#include "freelock_smem.hh"
#include <atomic>
#include <cstdint>
#include <cstdio>


namespace chrindex::andren::network
{
    #define COVER_SHMEMDATA_TYPE_AS_REAL(x)  \
        ( reinterpret_cast<ShDataPrivate_Real*>( x.reference()) )

    #define CONST_COVER_SHMEMDATA_TYPE_AS_REAL(x)  \
        ( reinterpret_cast<ShDataPrivate_Real const*>( x.reference_const()) )

    FreeLockShareMemWriter::FreeLockShareMemWriter(std::string const & path,bool owner ) : m_shared_mem(path,owner)
    {
        if (owner)
        {
            auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
            p->already_uid.exchange(-1,std::memory_order_seq_cst);
            p->current_uid.exchange(-1,std::memory_order_seq_cst);
            p->size=0;
        }
    }

    FreeLockShareMemWriter::FreeLockShareMemWriter(FreeLockShareMemWriter && _)
    {
        this->~FreeLockShareMemWriter();
        m_shared_mem = std::move(_.m_shared_mem);
    }

    FreeLockShareMemWriter::~FreeLockShareMemWriter()
    {
        auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        p->current_uid.exchange(-1,std::memory_order_seq_cst);
    }

   
    bool FreeLockShareMemWriter::valid() const 
    {
        return m_shared_mem.valid();
    }

    std::string FreeLockShareMemWriter::path()const
    {
        return m_shared_mem.path();
    }

    bool FreeLockShareMemWriter::writable() const
    {
        auto p = CONST_COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        auto a = p->current_uid.load(std::memory_order_seq_cst);
        auto b = p->already_uid.load(std::memory_order_seq_cst);
        return a == b;
    }

    ssize_t FreeLockShareMemWriter::write_some(std::string const & data)
    {
        auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        uint16_t size = std::min(sizeof(p->data),data.size());
        auto crrt = p->current_uid.load(std::memory_order_seq_cst);

        ::memcpy(p->data,data.c_str(), size);
        p->size = size;
        
        if(crrt == INT8_MAX)
        {
            p->current_uid.exchange(1,std::memory_order_seq_cst);
        }else 
        {
            p->current_uid.fetch_add(1,std::memory_order_seq_cst);
        }
        return size;
    }
    
    bool FreeLockShareMemWriter::readend_closed()const 
    {
        auto p = CONST_COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        return p->already_uid.load(std::memory_order_seq_cst) == -1;
    }



////////////////////////////////////

    FreeLockShareMemReader::FreeLockShareMemReader(std::string const & path,bool owner ) : m_shared_mem(path,owner)
    {
        if (owner)
        {
            auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
            p->already_uid.exchange(-1,std::memory_order_seq_cst);
            p->current_uid.exchange(-1,std::memory_order_seq_cst);
            p->size=0;
        }
    }

    FreeLockShareMemReader::FreeLockShareMemReader(FreeLockShareMemReader && _)
    {
        this->~FreeLockShareMemReader();
        m_shared_mem = std::move(_.m_shared_mem);
    }

    FreeLockShareMemReader::~FreeLockShareMemReader()
    {
        auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        p->already_uid.exchange(-1,std::memory_order_seq_cst);
    }

    bool FreeLockShareMemReader::valid() const 
    {
        return m_shared_mem.valid();
    }

    std::string FreeLockShareMemReader::path()const
    {
        return m_shared_mem.path();
    }

    bool FreeLockShareMemReader::readable() const
    {
        auto p = CONST_COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        auto a = p->current_uid.load(std::memory_order_relaxed) ;
        auto b = p->already_uid.load(std::memory_order_relaxed) ;
        return ( a > b )  ||  ( b == INT8_MAX &&  a == 1);
    }

    ssize_t FreeLockShareMemReader::read_some(std::string & data)
    {
        auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        data = std::string(p->data,p->size);
        p->already_uid.exchange(p->current_uid.load(std::memory_order_seq_cst),
            std::memory_order_seq_cst);
        return data.size();
    }

    bool FreeLockShareMemReader::writend_closed() const 
    {
        auto p = CONST_COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        return p->current_uid.load(std::memory_order_seq_cst) == -1;
    }
}


