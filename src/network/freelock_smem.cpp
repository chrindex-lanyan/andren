

#include "freelock_smem.hh"
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
            p->already_uid=-1;
            p->current_uid=-1;
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
        p->current_uid=-1;
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
        return p->current_uid == p->already_uid;
    }

    ssize_t FreeLockShareMemWriter::write_some(std::string const & data)
    {
        auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        uint16_t size = std::min(sizeof(p->data),data.size());
        decltype(ShDataPrivate_POD::current_uid) crrt = p->current_uid;

        ::memcpy(p->data,data.c_str(), size);
        p->size = size;
        
        if(crrt == INT8_MAX)
        {
            p->current_uid = 1;
        }else 
        {
            p->current_uid++;
        }
        return size;
    }
    
    bool FreeLockShareMemWriter::readend_closed()const 
    {
        auto p = CONST_COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        return p->already_uid == -1;
    }



////////////////////////////////////

    FreeLockShareMemReader::FreeLockShareMemReader(std::string const & path,bool owner ) : m_shared_mem(path,owner)
    {
        if (owner)
        {
            auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
            p->already_uid=-1;
            p->current_uid=-1;
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
        p->already_uid = -1;
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
        return (p->current_uid > p->already_uid) 
            || 
            ( p->already_uid ==INT8_MAX &&  p->current_uid == 1);
    }

    ssize_t FreeLockShareMemReader::read_some(std::string & data)
    {
        auto p = COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        data = std::string(p->data,p->size);
        p->already_uid = 0x00 | p->current_uid;
        return data.size();
    }

    bool FreeLockShareMemReader::writend_closed() const 
    {
        auto p = CONST_COVER_SHMEMDATA_TYPE_AS_REAL(m_shared_mem);
        return p->current_uid == -1;
    }
}


