#pragma once

#include <algorithm>
#include <cstddef>

namespace stdx {
    template <typename T>
    class FixedSizeDequeIterator;

    /// @brief Deque implemented on top of a fixed size array.
    ///
    /// @author Andrew Haisley (@andrewhaisley)
    ///
    /// @tparam T Type of the elements in the deque.
    template <typename T>
    class FixedSizeDeque {
    public:
        friend class FixedSizeDequeIterator<T>;

        typedef FixedSizeDequeIterator<T> iterator;

        FixedSizeDeque(T *entries, size_t size)
            : mCount(0)
            , mFront(0)
            , mBack(0)
            , mEntries(entries)
            , mCapacity(size)
        { }

        iterator begin() { return iterator(this, 0); }

        iterator end() { return iterator(this, mCount); }

        iterator begin() const { return iterator(this, 0); }

        iterator end() const { return iterator(this, mCount); }

        // returns true on success, false if the queue is full
        bool addFront(T e) {
            if (mCount == capacity()) {
                return false;
            }

            mEntries[mFront++] = e;
            mFront = mFront % capacity();
            mCount++;

            return true;
        }

        // returns true on success, false if the queue is empty (i.e. there is nothing
        // to pop)
        bool popFront() {
            if (mCount == 0) {
                return false;
            } else {
                mCount--;
            }

            if (mFront == 0) {
                mFront = capacity() - 1;
            } else {
                mFront--;
            }

            return true;
        }

        // returns true on success, false if the queue is full
        bool addBack(T e) {
            if (mCount == capacity()) {
                return false;
            }

            if (mBack == 0) {
                mBack = capacity() - 1;
            } else {
                mBack--;
            }

            mEntries[mBack] = e;
            mCount++;

            return true;
        }

        // returns true on success, false if the queue is empty (i.e. there is nothing
        // to pop)
        bool popBack() {
            if (mCount == 0) {
                return false;
            } else {
                mCount--;
            }

            mBack++;
            mBack = mBack % capacity();

            return true;
        }

        // Returns the entry at a given index. Behaviour is undefined if the index
        // is out of range.
        T &get(size_t index) const {
            if (mCount == 0 || index > mCount) {
                return mEntries[0];
            } else {
                return mEntries[(mBack + index) % capacity()];
            }
        }

        // Erases an entry at a given index - returns false if the index was out of
        // range
        bool remove(size_t index) {
            if (mCount == 0 || index > mCount) {
                return false;
            }

            index = (mBack + index) % capacity();

            if (mBack < mFront) {
                std::copy(&(mEntries[index + 1]), &(mEntries[mFront]), &(mEntries[index]));
            } else if (index >= mBack) {
                std::copy(&(mEntries[index + 1]), &(mEntries[capacity()]), &(mEntries[index]));
                std::copy(&(mEntries[0]), &(mEntries[1]), &(mEntries[capacity() - 1]));
                std::copy(&(mEntries[1]), &(mEntries[mFront]), &(mEntries[0]));
            } else {
                std::copy(&(mEntries[index + 1]), &(mEntries[mFront]), &(mEntries[index]));
            }

            (void)popFront();
            return true;
        }

        iterator erase(iterator it) {
            remove(it.mIndex);
            return iterator(this, it.mIndex);
        }

        T pollFront() {
            T result = front();
            popFront();
            return result;
        }

        T pollBack() {
            T result = back();
            popBack();
            return result;
        }

        T &front() { return *--end(); }
        T &back() { return *begin(); }

        const T &front() const { return *--end(); }
        const T &back() const { return *begin(); }

        size_t count() const { return mCount; }
        size_t capacity() const { return mCapacity; }

        bool isEmpty() const { return count() == 0; }
        bool isFull() const { return count() == capacity(); }

    private:
        size_t mCount;
        size_t mFront;
        size_t mBack;

        T *mEntries;
        size_t mCapacity;
    };

    template <typename T>
    class FixedSizeDequeIterator {
        friend class FixedSizeDeque<T>;

    public:
        using value_type = T;
        using difference_type = int;
        using pointer = T *;
        using reference = T &;
        using iterator_category = std::random_access_iterator_tag;

        FixedSizeDequeIterator(const FixedSizeDeque<T> *q, size_t index)
            : mContainer(q)
            , mIndex(index)
        { }

        FixedSizeDequeIterator<T> &operator--() {
            mIndex--;
            return *this;
        }

        FixedSizeDequeIterator<T> operator--(int) {
            FixedSizeDequeIterator<T> temp = *this;
            --mIndex;
            return temp;
        }

        T &operator*() const { return mContainer->get(mIndex); }

        FixedSizeDequeIterator<T> &operator++() {
            mIndex++;
            return *this;
        }

        FixedSizeDequeIterator<T> operator++(int) {
            FixedSizeDequeIterator temp = *this;
            ++mIndex;
            return temp;
        }

        FixedSizeDequeIterator<T> &operator+=(difference_type n) {
            mIndex += n;
            return *this;
        }

        FixedSizeDequeIterator<T> &operator-=(difference_type n) {
            mIndex -= n;
            return *this;
        }

        constexpr auto operator<=>(const FixedSizeDequeIterator<T> &other) const {
            return mIndex <=> other.mIndex;
        }

        constexpr bool operator==(const FixedSizeDequeIterator<T> &other) const {
            return mIndex == other.mIndex;
        }

        constexpr bool operator!=(const FixedSizeDequeIterator<T> &other) const {
            return mIndex != other.mIndex;
        }

        FixedSizeDequeIterator<T> operator+(difference_type n) const {
            FixedSizeDequeIterator temp = *this;
            temp += n;
            return temp;
        }

        FixedSizeDequeIterator<T> operator-(difference_type n) const {
            FixedSizeDequeIterator temp = *this;
            temp -= n;
            return temp;
        }

        friend difference_type operator-(const FixedSizeDequeIterator<T> &lhs,
                                        const FixedSizeDequeIterator<T> &rhs) {
            return lhs.mIndex - rhs.mIndex;
        }

    private:
        const FixedSizeDeque<T> *mContainer;
        size_t mIndex;
    };
}
