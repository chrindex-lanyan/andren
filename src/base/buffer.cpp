

#include "buffer.hh"

namespace chrindex ::andren::base
{
    buffer_t::buffer_t()
    {
        m_data.reserve(1024);
    }

    buffer_t::buffer_t(const buffer_t & b)
    {
        m_data = b.m_data;
    }

    buffer_t::buffer_t(buffer_t&& _b) noexcept
    {
        buffer_t b;
        b.m_data.swap(_b.m_data);
        b.m_data.swap(m_data);
    }

    buffer_t::buffer_t(const char * ptr , size_t size)
    {
        m_data.reserve(size);
        for(size_t i = 0; i<size ; i++)
        {
            m_data.push_back(size);
        }
    }

    buffer_t & buffer_t::operator=(const buffer_t &b)
    {
        m_data = b.m_data;
        return *this;
    }

    buffer_t & buffer_t::operator=(buffer_t &&b)
    {
        buffer_t t;
        t.m_data.swap(b.m_data);
        m_data.swap(t.m_data);
        return *this;
    }

    buffer_t::operator const char *() const noexcept
    {
        return &m_data[0];
    }

    buffer_t::operator bool() const noexcept
    {
        return !m_data.empty();
    }

    buffer_t & buffer_t::swap(buffer_t &_b) noexcept
    {
        _b.m_data.swap(m_data);
        return *this;
    }

    buffer_t & buffer_t::add_data(const char * ptr, size_t size)
    {
        m_data.reserve(m_data.size() + size);
        for(size_t i = 0; i< size ; i++)
        {
            m_data.push_back(ptr[i]);
        }
        return *this;
    }

    void buffer_t::resize(size_t size)
    {
        m_data.resize(size);
    }

    void buffer_t::reset(size_t size)
    {
        zero();
        m_data.resize(size);
    }

    void buffer_t::zero()
    {
        m_data.clear();
    }

    void buffer_t::zero(size_t size)
    {
        size = std::min(size,m_data.size());
        for(size_t i =0 ; i< size ; i++)
        {
            m_data[i] = 0;
        }
    }

    char & buffer_t::operator[](size_t index)
    {
        if(index >= m_data.size())
        {
            throw "index out flow.";
        }
        return m_data[index];
    }
}
