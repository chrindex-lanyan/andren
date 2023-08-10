#pragma once

namespace chrindex::andren::base
{
    /// @brief 简单的键值对容器。
    ///     写它是因为C++20 之后std::pair少了几个operator。
    /// @tparam KTy
    /// @tparam VTy
    template <typename _KTy, typename _VTy>
    class KVPair
    {
    public:
        using KTy = _KTy;
        using VTy = _VTy;
        using KVPair_t = KVPair<KTy, VTy>;

        KVPair() {}

        KVPair(KTy const &_key, VTy const &_val)
            : m_key(_key), m_val(_val) {}

        KVPair(KTy &&_key, VTy &&_val)
            : m_key(std::forward<KTy>(_key)), m_val(std::forward<VTy>(_val)) {}

        KVPair(const KVPair &_another)
        {
            m_key = _another.m_key;
            m_val = _another.m_val;
        }

        KVPair(KVPair &&_another)
        {
            m_key = std::move(_another.m_key);
            m_val = std::move(_another.m_val);
        }

        ~KVPair() {}

        KVPair_t &operator=(const KVPair_t &_another)
        {
            m_key = _another.m_key;
            m_val = _another.m_val;
            return *this;
        }

        KVPair_t &operator=(KVPair_t &&_another)
        {
            m_key = std::move(_another.m_key);
            m_val = std::move(_another.m_val);
            return *this;
        }

        KVPair_t &swap(KVPair_t &_another)
        {
            std::swap(m_key, _another.m_key);
            std::swap(m_val, _another.m_val);
            return *this;
        }

        const KTy key() const
        {
            return m_key;
        }

        VTy &value()
        {
            return m_val;
        }

    public:
        bool operator>(const KVPair_t &_another)
        {
            return m_key > _another.m_key;
        }

        bool operator<(const KVPair_t &_another)
        {
            return m_key < _another.m_key;
        }

        bool operator==(const KVPair_t &_another)
        {
            return m_key == _another.m_key;
        }

        bool operator!=(const KVPair_t &_another)
        {
            return m_key != _another.m_key;
        }

        bool operator>=(const KVPair_t &_another)
        {
            return m_key >= _another.m_key;
        }

        bool operator<=(const KVPair_t &_another)
        {
            return m_key <= _another.m_key;
        }

    private:
        KTy m_key;
        VTy m_val;
    };

}