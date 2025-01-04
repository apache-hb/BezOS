#pragma once

#include "std/std.hpp"

namespace stdx {
    template<typename T, size_t N>
    class StaticVector {
        union { T mStorage[N]; };
        size_t mSize;

        constexpr void init(const T *front, const T *back) {
            mSize = back - front;
            memcpy(mStorage, front, mSize * sizeof(T));
        }

    public:
        constexpr StaticVector()
            : mSize(0)
        { }

        constexpr StaticVector(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) {
            init(front, back);
        }

        constexpr ssize_t count() const { return mSize; }
        constexpr ssize_t capacity() const { return N; }
        constexpr ssize_t sizeInBytes() const { return mSize * sizeof(T); }
        constexpr bool isEmpty() const { return mSize == 0; }
        constexpr bool isFull() const { return mSize == N; }

        constexpr T *begin() { return mStorage; }
        constexpr T *end() { return mStorage + mSize; }

        constexpr const T *begin() const { return mStorage; }
        constexpr const T *end() const { return mStorage + mSize; }

        constexpr void clear() { mSize = 0; }

        constexpr void erase(const T& value) {
            for (size_t i = 0; i < mSize; i++) {
                if (mStorage[i] == value) {
                    remove(i);
                    return;
                }
            }
        }

        constexpr void remove(size_t index) {
            if (index < mSize) {
                for (size_t i = index; i < mSize - 1; i++)
                    mStorage[i] = mStorage[i + 1];
                mSize -= 1;
            }
        }

        constexpr void insert(size_t index, T value) {
            if (index < mSize) {
                for (size_t i = mSize; i > index; i--)
                    mStorage[i] = mStorage[i - 1];
                mStorage[index] = value;
                mSize += 1;
            }
        }

        constexpr T& operator[](size_t index) {
            return mStorage[index];
        }

        constexpr const T& operator[](size_t index) const {
            return mStorage[index];
        }

        constexpr bool add(const T& value) {
            if (isFull())
                return false;

            mStorage[mSize++] = value;
            return true;
        }

        constexpr void pop() {
            if (!isEmpty())
                mSize -= 1;
        }
    };
}