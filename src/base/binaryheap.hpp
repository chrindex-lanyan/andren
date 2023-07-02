#pragma once

#include <optional>
#include <vector>
#include <stdint.h>

namespace chrindex::andren::base
{

    /// @brief 数组二叉树实现的最小堆
    /// @tparam KTy 键
    /// @tparam VTy 值
    template <typename _KTy, typename _VTy>
    class BinaryHeap
    {
    public:
        using KTy = _KTy;
        using VTy = _VTy;
        using pair_t = KVPair<KTy, VTy>;
        using opt_pair_t = std::optional<pair_t>;

        BinaryHeap()
        {
            m_data.reserve(32);
        }

        BinaryHeap(const BinaryHeap<KTy, VTy> &_another)
        {
            m_data = _another.m_data;
        }

        BinaryHeap(BinaryHeap<KTy, VTy> &&_another)
        {
            m_data = std::move(_another.m_data);
        }

        BinaryHeap<KTy, VTy> &operator=(const BinaryHeap<KTy, VTy> &_another)
        {
            m_data = _another.m_data;
            return *this;
        }

        BinaryHeap<KTy, VTy> &operator=(BinaryHeap<KTy, VTy> &&_another)
        {
            m_data = std::move(_another.m_data);
            return *this;
        }

        ~BinaryHeap() {}

        BinaryHeap<KTy, VTy> &swap(BinaryHeap<KTy, VTy> &_another)
        {
            std::swap(m_data, _another.m_data);
        }

        void push(KTy &&key, VTy &&val)
        {
            push(KVPair{std::forward<KTy>(key), std::forward<VTy>(val)});
        }

        void push(pair_t &&_kv)
        {
            m_data.push_back(std::forward<pair_t>(_kv));
            sift_up(size() - 1);
        }

        opt_pair_t min_pair()
        {
            if (size() > 0)
            {
                return opt_pair_t{m_data[0]};
            }
            return opt_pair_t{};
        }

        opt_pair_t extract_min_pair()
        {
            if (size() <= 0)
            {
                return opt_pair_t{};
            }
            auto ret = opt_pair_t{std::move(m_data[0])};
            auto last = std::move(m_data[size() - 1]);
            m_data.pop_back();
            if (size() > 0)
            {
                m_data[0] = std::move(last);
                sift_down(0);
            }
            return ret;
        }

        void clear()
        {
            m_data.clear();
        }

        uint64_t size() const
        {
            return m_data.size();
        }

    private:
        uint64_t parent(uint64_t i) const
        {
            return (i - 1) / 2;
        }

        uint64_t left(uint64_t i) const
        {
            return i * 2 + 1;
        }

        uint64_t right(uint64_t i) const
        {
            return i * 2 + 2;
        }

        void sift_up(uint64_t i)
        {
            while (i > 0 && m_data[i] < m_data[parent(i)])
            {
                std::swap(m_data[i], m_data[parent(i)]);
                i = parent(i);
            }
        }

        void sift_down(uint64_t i)
        {
            for (;;)
            {
                auto mi = i;
                auto li = left(i);
                auto ri = right(i);

                if (li < size() && m_data[li] < m_data[mi])
                {
                    mi = li;
                }

                if (ri < size() && m_data[ri] < m_data[mi])
                {
                    mi = ri;
                }

                if (i != mi)
                {
                    std::swap(m_data[i], m_data[mi]);
                    i = mi;
                }
                else
                {
                    break;
                }
            }
        }

    private:
        std::vector<pair_t> m_data;
    };

}