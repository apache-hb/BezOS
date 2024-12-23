#pragma once

#include "std/std.hpp"

#include "std/string_view.hpp"

namespace stdx {
    template<typename T, size_t N>
    class StaticStringBase {
        T mStorage[N];
        ssize_t mSize;

        constexpr void init(const T *front, const T *back) {
            mSize = back - front;
            memcpy(mStorage, front, mSize * sizeof(T));
        }

    public:
        constexpr StaticStringBase()
            : mSize(0)
        { }

        template<size_t S> requires (S <= N)
        constexpr StaticStringBase(const T (&str)[S])
            : StaticStringBase(str, str + S)
        { }

        constexpr StaticStringBase(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) {
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

        constexpr T& operator[](size_t index) {
            return mStorage[index];
        }

        constexpr const T& operator[](size_t index) const {
            return mStorage[index];
        }

        constexpr operator StringViewBase<T>() const {
            return StringViewBase<T>(mStorage, mStorage + mSize);
        }

        constexpr bool operator==(StringViewBase<T> other) const {
            return count() == other.count() && memcmp(begin(), other.begin(), sizeInBytes()) == 0;
        }
    };

    template<size_t N>
    using StaticString = StaticStringBase<char, N>;
}
