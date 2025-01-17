#pragma once

#include <cstddef>

namespace stdx {
    template<typename T>
    class FixedDeque {
        /// @brief Start of deque memory.
        T *mFront;

        /// @brief End of deque memory.
        T *mBack;

        /// @brief Current deque head.
        T *mHead;

        /// @brief Current deque tail.
        T *mTail;

        /// @brief The current number of elements in the deque.
        /// @note This is annoying to have to keep track of, there is
        ///       probably a way to calculate this while ensuring
        ///       we can still differentiate between an empty and full deque.
        size_t mCount = 0;

        T *after(T *ptr) const {
            ptr += 1;
            if (ptr >= mBack) {
                return mFront;
            } else {
                return ptr;
            }
        }

        T *before(T *ptr) const {
            ptr -= 1;
            if (ptr < mFront) {
                return mBack - 1;
            } else {
                return ptr;
            }
        }

    public:
        constexpr FixedDeque(T *front, T *back)
            : mFront(front)
            , mBack(back)
            , mHead(front)
            , mTail(front)
        { }

        class Iterator {
            const FixedDeque *mContainer;

            T *mIter;

        public:
            using difference_type = std::ptrdiff_t;

            constexpr Iterator()
                : mContainer(nullptr)
                , mIter(nullptr)
            { }

            constexpr Iterator(const FixedDeque *container, T *iter)
                : mContainer(container)
                , mIter(iter)
            { }

            // fulfill the requirements of LegacyInputIterator

            constexpr bool operator!=(const Iterator& other) const {
                return mIter != other.mIter;
            }

            constexpr Iterator& operator++() {
                mIter = mContainer->after(mIter);
                return *this;
            }

            constexpr T& operator*() {
                return *mIter;
            }

            constexpr difference_type operator-(const Iterator& other) const {
                if (other.mIter <= mIter) {
                    return mIter - other.mIter;
                } else {
                    return mContainer->mBack - other.mIter + mIter - mContainer->mFront;
                }
            }

            constexpr Iterator& operator+=(difference_type n) {
                mIter = mContainer->after(mIter, n);
                return *this;
            }
        };

        Iterator begin() { return Iterator(this, mHead - 1); }
        Iterator end() { return Iterator(this, mTail); }

        Iterator begin() const { return Iterator(this, mHead - 1); }
        Iterator end() const { return Iterator(this, mTail); }

        bool isEmpty() const { return mHead == mTail; }
        bool isFull() const { return count() == capacity(); }

        T& head() { return *mHead; }
        T& tail() { return *mTail; }

        size_t capacity() const { return mBack - mFront; }
        size_t count() const { return mCount; }

        bool addBack(T value) {
            if (isFull()) {
                return false;
            }

            mCount += 1;
            *mHead = value;
            mHead = before(mHead);
            return true;
        }

        bool addFront(T value) {
            if (isFull()) {
                return false;
            }

            mCount += 1;
            mTail = after(mTail);
            *mTail = value;
            return true;
        }

        T pollFront() {
            mCount -= 1;
            T value = *mTail;
            mTail = before(mTail);
            return value;
        }

        T pollBack() {
            mCount -= 1;
            mHead = after(mHead);
            return *mHead;
        }

        void erase(Iterator it) {
            Iterator next = it;
            while (next != end()) {
                *it = *next;
                it++;
                next++;
            }
            mTail = before(mTail);
            mCount -= 1;
        }
    };
}
