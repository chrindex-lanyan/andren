#pragma once

#include <stdint.h>
#include <utility>
#include <vector>
#include <optional>


namespace chrindex::andren::base
{
    /// 简单的环形队列。
    /// 该队列使用std::vector模拟，且自动扩容。
    template <typename T>
    class CircularQueue 
    {
    public:
         
        using CircularQueue_Type = CircularQueue<T>;

        ~CircularQueue() = default;

        CircularQueue() 
        {
            m_vec.resize(16);
            m_current = 0; 
            m_head = 0;
        }

        CircularQueue(size_t presize) 
        {
            m_vec.resize(std::max(presize, static_cast<size_t>(16)));
            m_current = 0;
            m_head = 0;
        }
        CircularQueue(CircularQueue_Type&& _) noexcept
        {
            m_vec = std::move(_.m_vec);
            m_current = _.m_current;
            m_head = _.m_head;
        }

        CircularQueue(CircularQueue_Type const & _)
        {
            m_vec =_.m_vec;
            m_current = _.m_current;
            m_head = _.m_head;
        }

        void operator=(CircularQueue_Type&& _) noexcept
        {
            m_vec = std::move(_.m_vec);
            m_current = _.m_current;
            m_head = _.m_head;
            _.m_current = 0;
            _.m_head = 0;
        }

        void operator=(CircularQueue_Type const & _) 
        {
            m_vec = _.m_vec;
            m_current = _.m_current;
            m_head = _.m_head;
        }

        void push_back(const T& value) 
        {
            auto v = value;
            emplace_back(std::move(v));
        }

        void emplace_back(T && value)
        {
            add(std::move(value));
        }

        std::optional<T> take() 
        {
            return pop();
        }

        T& front()
        {
            return m_vec[m_head];
        }

        std::optional<T> pop_front()
        {
            return pop();
        }

        bool empty() const 
        {
            return m_head == m_current;
        }

        size_t capacity() const 
        {
            return m_vec.size();
        }

        void swap(CircularQueue_Type & _)
        {
            std::swap(m_vec,_.m_vec);
            std::swap(m_current , _.m_current);
            std::swap(m_head , _.m_head);
        }
        
        void clear()
        {
            size_t _capacity = m_vec.size();
            m_current = 0;
            m_head = 0;
            m_vec.clear();
            m_vec.resize(_capacity);
        }

    private :

        void add(T && v) 
        {
            if (m_head < m_current) ///   0 ... head ... current ... max_size
            {
                m_vec[m_current] = std::forward<T>(v);
                if (m_current == m_vec.size() - 1)  // 到vector最后的位置是空闲了。
                {
                    if (m_head == 0) // 该扩容 。   0 == head ..... current == old_size
                    {
                        m_current++;
                        m_vec.resize(m_vec.size()*2);
                        // now  :  0 == head ... old_size , current ...[empty]... new_size 
                    }
                    else // if( m_head > 0) 尚且无需扩容。 0 ...[empty]...  head ..... current == old_size
                    {
                        m_current = 0;
                        // now : current ...[empty]...  head ... old_size
                    }
                }
                else  // 没到vector的最后位置，自增即可。 0 ... head ... current ...[empty]... old_size
                {
                    m_current++;
                    // now : 0 ... head ... current ...[empty]... old_size
                }
                return;
            }
            else if (m_head == m_current) [[unlikely]] ///    0 ... head == current ... max_size
            {
                if (m_current == (m_vec.size() -1)) ///  0 ...[empty]... head == current == max_size
                {
                    m_vec[m_current] = std::move(v);
                    m_current = 0;
                    return;
                }
                else ///  0 ...[empty]... head == current ...[empty]... max_size
                {
                    m_vec[m_current] = std::move(v);
                    m_current++;
                    return;
                }
            }
            else // if(m_head > m_current) /// 0 ... current ... head ... max_size
            {
                m_vec[m_current] = std::move(v);
                if (m_current == (m_head -1)) // add完这个就满了，需要扩容。 0 ... current , head ... max_size
                {
                    //  0 ... current == head ... max_size
                    std::vector<T> tmp;
                    // 考虑到T可能是大对象，还是手动Move一下吧。
                    for (size_t i = m_head; i < m_vec.size(); i++)
                    {
                        tmp.emplace_back(std::move(m_vec[i]));
                    }
                    for (size_t i = 0; i < m_head; i++)
                    {
                        tmp.emplace_back(std::move(m_vec[i]));
                    }
                    m_head = 0;
                    m_current = m_vec.size();
                    tmp.resize(std::max(tmp.size() * 2, static_cast<size_t>(1)));
                    m_vec = std::move(tmp);
                    //  0 == head ... current ... new_size
                }
                else  // 无需扩容。 0 ... current ... head ... max_size
                {
                    m_current++;
                }
                return;
            }
        }

        std::optional<T> pop()
        {
            if (m_head < m_current) ///   0 ... head ... current ... max_size
            {
                T v = std::move(m_vec[m_head]);
                m_head++;
                return v;
            }
            else if (m_head == m_current) [[unlikely]] ///    0 ... head == current ... max_size
            {
                // empty 
                return {};
            }
            else // if(m_head > m_current) /// 0 ... current ... head ... max_size
            {
                T v = std::move(m_vec[m_head]);
                if (m_head == (m_vec.size()-1)) [[unlikely]]
                {
                    m_head = 0;
                }
                else [[likely]]
                {
                    m_head++;
                }
                return v;
            }
        }

    private:
        std::vector<T> m_vec;
        size_t m_current; // 下一个空置的位置
        size_t m_head; // 头标
    };


    template<typename T>
    using circular_queue = CircularQueue<T>;

}

