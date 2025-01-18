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
        T *mTail;

        /// @brief Current deque tail.
        T *mHead;

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
            , mTail(front)
            , mHead(front)
        { }

        class Iterator {
            FixedDeque *mContainer;

            size_t mIndex;

        public:
            using difference_type = std::ptrdiff_t;

            constexpr Iterator()
                : mContainer(nullptr)
                , mIndex(0)
            { }

            constexpr Iterator(const FixedDeque *container, size_t iter)
                : mContainer(container)
                , mIndex(iter)
            { }

            constexpr bool operator!=(const Iterator& other) const {
                return mIndex != other.mIndex;
            }

            constexpr bool operator==(const Iterator& other) const {
                return mIndex == other.mIndex;
            }

            constexpr Iterator& operator++() {
                mIndex++;
                return *this;
            }

            constexpr Iterator operator++(int) {
                Iterator copy = *this;
                mIndex++;
                return copy;
            }

            constexpr Iterator& operator--() {
                mIndex--;
                return *this;
            }

            constexpr Iterator operator--(int) {
                Iterator copy = *this;
                mIndex--;
                return copy;
            }

            constexpr T& operator*() {
                return mContainer->get(mIndex);
            }

            constexpr difference_type operator-(const Iterator& other) const {
                return mIndex - other.mIndex;
            }

            constexpr Iterator operator+(difference_type diff) const {
                return Iterator(mContainer, mIndex + diff);
            }
        };

        Iterator end() {
            return Iterator(this, mTail);
        }

        Iterator begin() {
            return Iterator(this, mHead);
        }

        Iterator end() const {
            return Iterator(this, mTail);
        }

        Iterator begin() const {
            return Iterator(this, mHead);
        }

        bool isEmpty() const { return mCount == 0; }
        bool isFull() const { return count() == capacity(); }

        T& back() { return *mTail; }
        T& front() { return *mHead; }

        size_t capacity() const { return mBack - mFront; }
        size_t count() const { return mCount; }

        bool addBack(T value) {
            if (isFull()) {
                return false;
            }

            mCount += 1;
            *mTail = value;
            mTail = before(mTail);
            return true;
        }

        bool addFront(T value) {
            if (isFull()) {
                return false;
            }

            mCount += 1;
            mHead = after(mHead);
            *mHead = value;
            return true;
        }

        T pollFront() {
            mCount -= 1;
            T value = *mHead;
            mHead = before(mHead);
            return value;
        }

        T pollBack() {
            mCount -= 1;
            mTail = after(mTail);
            return *mTail;
        }

        T &get(size_t index) {
            T *ptr = mHead;
            for (size_t i = 0; i < index; i++) {
                ptr = before(ptr);
            }
            return *ptr;
        }

        const T &get(size_t index) const {
            T *ptr = mHead;
            for (size_t i = 0; i < index; i++) {
                ptr = before(ptr);
            }
            return *ptr;
        }

        Iterator erase(Iterator it) {
            Iterator current = it;

            while (current != end()) {
                *current = *++current;
            }

            mCount -= 1;
            mHead = before(mHead);

            return it;
        }
    };
}
