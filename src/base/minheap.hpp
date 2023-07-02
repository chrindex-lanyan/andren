#pragma once


namespace chrindex::andren::base
{
    /// @brief 依赖于二叉堆或四叉堆的最小堆
    /// @tparam HeapTy
    template <typename _HeapTy>
    class MinHeap
    {
    public:
        using HeapTy = _HeapTy;

        MinHeap() {}

        ~MinHeap() {}

        MinHeap(const MinHeap<HeapTy> &_)
        {
            m_heap = _.m_heap;
        }

        MinHeap(MinHeap<HeapTy> &&_)
        {
            m_heap = std::move(_.m_heap);
        }

        MinHeap<HeapTy> &operator=(const MinHeap<HeapTy> &_)
        {
            m_heap = _.m_heap;
            return *this;
        }

        MinHeap<HeapTy> &operator=(MinHeap<HeapTy> &&_)
        {
            m_heap = std::move(_.m_heap);
            return *this;
        }

    public:
        MinHeap<HeapTy> &swap(MinHeap<HeapTy> &_another)
        {
            std::swap(m_heap, _another.m_heap);
        }

        void push(typename HeapTy::KTy &&key, typename HeapTy::VTy &&val)
        {
            m_heap.push(std::forward<typename HeapTy::KTy>(key), std::forward<typename HeapTy::VTy>(val));
        }

        void push(typename HeapTy::pair_t &&_kv)
        {
            m_heap.push(std::forward<typename HeapTy::pair_t>(_kv));
        }

        typename HeapTy::opt_pair_t min_pair()
        {
            return m_heap.min_pair();
        }

        typename HeapTy::opt_pair_t extract_min_pair()
        {
            return m_heap.extract_min_pair();
        }

        void clear()
        {
            m_heap.clear();
        }

        uint64_t size() const
        {
            return m_heap.size();
        }

    private:
        HeapTy m_heap;
    };
}