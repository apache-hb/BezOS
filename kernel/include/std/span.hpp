#pragma once

#include "std/std.hpp"

namespace stdx {
    template<typename T>
    class Span {
        T *mFront;
        T *mBack;

    public:
        constexpr Span()
            : Span(nullptr, nullptr)
        { }

        template<size_t N>
        constexpr Span(T (&array)[N])
            : Span(array, array + N)
        { }

        constexpr Span(T *front [[gnu::nonnull]], T *back [[gnu::nonnull]])
            : mFront(front)
            , mBack(back)
        { }

        constexpr Span(T *front [[gnu::nonnull]], size_t count)
            : mFront(front)
            , mBack(front + count)
        { }

        constexpr ssize_t count() const { return mBack - mFront; }
        constexpr ssize_t sizeInBytes() const { return count() * sizeof(T); }
        constexpr bool isEmpty() const { return mBack == mFront; }

        constexpr T *begin() { return mFront; }
        constexpr T *end() { return mBack; }

        constexpr const T *begin() const { return mFront; }
        constexpr const T *end() const { return mBack; }

        constexpr T& operator[](size_t index) {
            return mFront[index];
        }

        constexpr const T& operator[](size_t index) const {
            return mFront[index];
        }
    };
}