#pragma once

#include "std/std.hpp"

#include "std/string_view.hpp"

namespace stdx {
    template<typename T, size_t N>
    class StaticStringBase {
        T mStorage[N];
        ssize_t mSize;

        constexpr void init(const T *front, const T *back) noexcept {
            mSize = back - front;
            memcpy(mStorage, front, mSize * sizeof(T));
        }

    public:
        constexpr StaticStringBase() noexcept
            : mSize(0)
        { }

        template<size_t S> requires (S <= N)
        constexpr StaticStringBase(const T (&str)[S]) noexcept
            : StaticStringBase(str, str + S)
        { }

        constexpr StaticStringBase(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) noexcept {
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

        constexpr T& operator[](size_t index) noexcept {
            return mStorage[index];
        }

        constexpr const T& operator[](size_t index) const noexcept {
            return mStorage[index];
        }

        constexpr operator StringViewBase<T>() const noexcept {
            return StringViewBase<T>(mStorage, mStorage + mSize);
        }

        constexpr bool operator==(StringViewBase<T> other) const noexcept {
            return count() == other.count() && memcmp(begin(), other.begin(), sizeInBytes()) == 0;
        }
    };

    template<size_t N>
    using StaticString = StaticStringBase<char, N>;
}
