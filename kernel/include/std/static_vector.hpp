#pragma once

#include "std/std.hpp"

namespace stdx {
    template<typename T, size_t N>
    class StaticVector {
        T mStorage[N];
        size_t mSize;

        constexpr void init(const T *front, const T *back) noexcept {
            mSize = back - front;
            memcpy(mStorage, front, mSize * sizeof(T));
        }

    public:
        constexpr StaticVector() noexcept
            : mSize(0)
        { }

        constexpr StaticVector(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) noexcept {
            init(front, back);
        }

        constexpr ssize_t count() const noexcept { return mSize; }
        constexpr ssize_t capacity() const noexcept { return N; }
        constexpr ssize_t sizeInBytes() const noexcept { return mSize * sizeof(T); }
        constexpr bool isEmpty() const noexcept { return mSize == 0; }
        constexpr bool isFull() const noexcept { return mSize == N; }

        constexpr T *begin() noexcept { return mStorage; }
        constexpr T *end() noexcept { return mStorage + mSize; }

        constexpr const T *begin() const noexcept { return mStorage; }
        constexpr const T *end() const noexcept { return mStorage + mSize; }

        constexpr void clear() noexcept { mSize = 0; }

        constexpr void remove(size_t index) noexcept {
            if (index < mSize) {
                for (size_t i = index; i < mSize - 1; i++)
                    mStorage[i] = mStorage[i + 1];
                mSize -= 1;
            }
        }

        constexpr void insert(size_t index, T value) noexcept {
            if (index < mSize) {
                for (size_t i = mSize; i > index; i--)
                    mStorage[i] = mStorage[i - 1];
                mStorage[index] = value;
                mSize += 1;
            }
        }

        constexpr T& operator[](size_t index) noexcept {
            return mStorage[index];
        }

        constexpr const T& operator[](size_t index) const noexcept {
            return mStorage[index];
        }

        constexpr bool add(const T& value) noexcept {
            if (isFull())
                return false;

            mStorage[mSize++] = value;
            return true;
        }

        constexpr void pop() noexcept {
            if (!isEmpty())
                mSize -= 1;
        }
    };
}