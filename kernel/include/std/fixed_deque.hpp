#pragma once

#include <algorithm>
#include <cstddef>

template <typename T>
class fsdeque_iterator;

template <typename T>
class fsdeque
{
public:

    friend class fsdeque_iterator<T>;

    typedef fsdeque_iterator<T> iterator;

    fsdeque(T *entries, size_t max_entries)
    {
        m_num_entries = 0;
        m_front = 0;
        m_back = 0;
        m_entries = entries;
        m_max_entries = max_entries;
    }

    iterator begin()
    {
        return iterator(*this, 0);
    }

    iterator end()
    {
        return iterator(*this, m_num_entries);
    }

    // returns true on success, false if the queue is full
    bool push_front(T e)
    {
        if (m_num_entries == capacity())
        {
            return false;
        }

        m_entries[m_front++] = e;
        m_front = m_front % capacity();
        m_num_entries++;

        return true;
    }

    // returns true on success, false if the queue is empty (i.e. there is nothing to pop)
    bool pop_front()
    {
        if (m_num_entries == 0)
        {
            return false;
        }
        else
        {
            m_num_entries--;
        }

        if (m_front == 0)
        {
            m_front = capacity() - 1;
        }
        else
        {
            m_front--;
        }

        return true;
    }

    // returns true on success, false if the queue is full
    bool push_back(T e)
    {
        if (m_num_entries == capacity())
        {
            return false;
        }

        if (m_back == 0)
        {
            m_back = capacity() - 1;
        }
        else
        {
            m_back--;
        }

        m_entries[m_back] = e;
        m_num_entries++;

        return true;
    }

    // returns true on success, false if the queue is empty (i.e. there is nothing to pop)
    bool pop_back()
    {
        if (m_num_entries == 0)
        {
            return false;
        }
        else
        {
            m_num_entries--;
        }

        m_back++;
        m_back = m_back % capacity();

        return true;
    }

    // Returns the entry at a given index. Behaviour is undefined if the index
    // is out of range.
    T &get_entry(size_t index) const
    {
        if (m_num_entries == 0 || index > m_num_entries)
        {
            return m_entries[0];
        }
        else
        {
            return m_entries[(m_back + index) % capacity()];
        }
    }

    // Erases an entry at a given index - returns false if the index was out of range
    bool remove(size_t index)
    {
        if (m_num_entries == 0 || index > m_num_entries)
        {
            return false;
        }
        else
        {
            index = (m_back + index) % capacity();

            if (m_back < m_front)
            {
                std::copy(&(m_entries[index + 1]), &(m_entries[m_front]), &(m_entries[index]));

            }
            else
            {
                if (index >= m_back)
                {
                    std::copy(&(m_entries[index + 1]), &(m_entries[capacity()]), &(m_entries[index]));
                    std::copy(&(m_entries[0]), &(m_entries[1]), &(m_entries[capacity() - 1]));
                    std::copy(&(m_entries[1]), &(m_entries[m_front]), &(m_entries[0]));
                }
                else
                {
                    std::copy(&(m_entries[index + 1]), &(m_entries[m_front]), &(m_entries[index]));
                }
            }

            (void)pop_front();
            return true;
        }
    }

    iterator erase(iterator it)
    {
        remove(it.m_index);
        return iterator(*this, it.m_index);
    }

    T poll_front()
    {
        T result = front();
        pop_front();
        return result;
    }

    T poll_back()
    {
        T result = back();
        pop_back();
        return result;
    }

    T &front()
    {
        return *--end();
    }

    T &back()
    {
        return *begin();
    }

    size_t count() const
    {
        return m_num_entries;
    }

    size_t capacity() const
    {
        return m_max_entries;
    }

    bool isEmpty() const { return count() == 0; }
    bool isFull() const { return count() == capacity(); }

private:

    size_t m_num_entries;
    size_t m_front;
    size_t m_back;

    T *m_entries;
    size_t m_max_entries;
};

template <typename T>
class fsdeque_iterator
{
    friend class fsdeque<T>;
public:
    using value_type = T;
    using difference_type = int;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::random_access_iterator_tag;

    fsdeque_iterator(const fsdeque<T> &q, size_t index) : m_fsdeque(&q), m_index(index)
    {
    }

    fsdeque_iterator<T> &operator--()
    {
        m_index--;
        return *this;
    }

    fsdeque_iterator<T> operator--(int)
    {
        fsdeque_iterator<T> temp = *this;
        --m_index;
        return temp;
    }

    T &operator*() const
    {
        return m_fsdeque->get_entry(m_index);
    }

    fsdeque_iterator<T> &operator++()
    {
        m_index++;
        return *this;
    }

    fsdeque_iterator<T> operator++(int)
    {
        fsdeque_iterator temp = *this;
        ++m_index;
        return temp;
    }

    fsdeque_iterator<T> &operator+=(difference_type n)
    {
        m_index += n;
        return *this;
    }

    fsdeque_iterator<T> &operator-=(difference_type n)
    {
        m_index -= n;
        return *this;
    }

    bool operator==(const fsdeque_iterator<T>& other) const
    {
        return m_index == other.m_index;
    }

    bool operator!=(const fsdeque_iterator<T>& other) const
    {
        return m_index != other.m_index;
    }

    bool operator<(const fsdeque_iterator<T>& other) const
    {
        return m_index < other.m_index;
    }

    fsdeque_iterator<T> operator+(difference_type n) const
    {
        fsdeque_iterator temp = *this;
        temp += n;
        return temp;
    }

    fsdeque_iterator<T> operator-(difference_type n) const
    {
        fsdeque_iterator temp = *this;
        temp -= n;
        return temp;
    }

    friend difference_type operator-(const fsdeque_iterator<T> &lhs, const fsdeque_iterator<T> &rhs)
    {
        return lhs.m_index - rhs.m_index;
    }

private:
    const fsdeque<T> *m_fsdeque;
    size_t m_index;

};
