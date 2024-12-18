#pragma once

#include "std/std.hpp"

namespace stdx {
    template<typename T>
    class Span {
        T *mFront;
        T *mBack;

    public:
        constexpr Span() noexcept
            : Span(nullptr, nullptr)
        { }

        template<size_t N>
        constexpr Span(T (&array)[N]) noexcept
            : Span(array, array + N)
        { }

        constexpr Span(T *front [[gnu::nonnull]], T *back [[gnu::nonnull]]) noexcept
            : mFront(front)
            , mBack(back)
        { }

        constexpr Span(T *front [[gnu::nonnull]], size_t count) noexcept
            : mFront(front)
            , mBack(front + count)
        { }

        constexpr ssize_t count() const noexcept { return mBack - mFront; }
        constexpr ssize_t sizeInBytes() const noexcept { return count() * sizeof(T); }
        constexpr bool isEmpty() const noexcept { return mBack == mFront; }

        constexpr T *begin() noexcept { return mFront; }
        constexpr T *end() noexcept { return mBack; }

        constexpr const T *begin() const noexcept { return mFront; }
        constexpr const T *end() const noexcept { return mBack; }

        constexpr T& operator[](size_t index) noexcept {
            return mFront[index];
        }

        constexpr const T& operator[](size_t index) const noexcept {
            return mFront[index];
        }
    };
}