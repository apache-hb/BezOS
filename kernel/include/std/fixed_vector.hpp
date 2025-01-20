#pragma once

#include <cstddef>
#include <memory>

namespace stdx {
    template<typename T>
    class FixedVector {
        T *mFront;
        T *mBack;
        T *mCapacity;

    public:
        FixedVector(T *front, T *back)
            : mFront(front)
            , mBack(front)
            , mCapacity(back)
        { }

        size_t capacity() const { return mCapacity - mFront; }
        size_t count() const { return mBack - mFront; }

        bool isEmpty() const { return mFront == mBack; }
        bool isFull() const { return mBack == mCapacity; }

        T& operator[](size_t index) { return mFront[index]; }
        const T& operator[](size_t index) const { return mFront[index]; }

        T *begin() { return mFront; }
        const T *begin() const { return mFront; }

        T *end() { return mBack; }
        const T *end() const { return mBack; }

        T& front() { return *mFront; }
        const T& front() const { return *mFront; }

        T& back() { return *(mBack - 1); }
        const T& back() const { return *(mBack - 1); }

        bool add(T value) {
            if (mBack < mCapacity) {
                std::construct_at(mBack++, value);
                return true;
            }

            return false;
        }

        void clear() {
            std::destroy(begin(), end());
            mBack = mFront;
        }

        T pop() {
            T value = *--mBack;
            std::destroy_at(mBack);
            return value;
        }

        void erase(T *it) {
            std::destroy_at(it);
            std::move(it + 1, mBack--, it);
        }
    };
}
