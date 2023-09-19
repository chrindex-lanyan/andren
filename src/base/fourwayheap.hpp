#pragma once

#include <functional>
#include <optional>
#include <vector>
#include <stdint.h>

#include "KVPair.hpp"

namespace chrindex::andren::base
{

    /// @brief 简单的四叉树最小堆容器
    /// @tparam KTy 键
    /// @tparam VTy 值
    template <typename _KTy, typename _VTy>
    class FourWayHeap
    {
    public:
        using KTy = _KTy;
        using VTy = _VTy;
        using pair_t = KVPair<KTy, VTy>;
        using opt_pair_t = std::optional<pair_t>;

        FourWayHeap()
        {
            m_data.reserve(32);
        }

        FourWayHeap(const FourWayHeap<KTy, VTy> &_another)
        {
            m_data = _another.m_data;
        }

        FourWayHeap(FourWayHeap<KTy, VTy> &&_another)
        {
            m_data = std::move(_another.m_data);
        }

        FourWayHeap<KTy, VTy> &operator=(const FourWayHeap<KTy, VTy> &_another)
        {
            m_data = _another.m_data;
            return *this;
        }

        FourWayHeap<KTy, VTy> &operator=(FourWayHeap<KTy, VTy> &&_another)
        {
            m_data = std::move(_another.m_data);
            return *this;
        }

        ~FourWayHeap() {}

        FourWayHeap<KTy, VTy> &swap(FourWayHeap<KTy, VTy> &_another)
        {
            std::swap(m_data, _another.m_data);
            return *this;
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

        void foreach_pair(std::function<void(pair_t &)> cb)
        {
            if (!cb)
            {
                return ;
            }
            for (auto & pair : m_data)
            {
                cb(pair);
            }
        }

        void foreach_pair_const(std::function<void(pair_t const&)> cb)
        {
            if (!cb)
            {
                return ;
            }
            for (auto const & pair : m_data)
            {
                cb(pair);
            }
        }

    private:
        uint64_t parent(uint64_t i) const
        {
            return (i - 1) / 4;
        }

        uint64_t child(uint64_t i, uint64_t k) const
        {
            return i * 4 + k;
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

                for (uint64_t k = 1; k < 5; k++)
                {
                    auto ci = child(i, k);
                    if (ci < size() && m_data[ci] < m_data[mi])
                        mi = ci;
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