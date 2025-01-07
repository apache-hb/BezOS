#pragma once

#include "std/std.hpp"

#include "std/traits.hpp"

#include <iterator>

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
            : Span(std::begin(array), std::end(array))
        { }

        template<IsRange<T> R>
        constexpr Span(R&& range)
            : Span(range.begin(), range.end())
        { }

        constexpr Span(T *front [[gnu::nonnull]], T *back [[gnu::nonnull]])
            : mFront(front)
            , mBack(back)
        { }

        constexpr size_t count() const { return mBack - mFront; }
        constexpr size_t sizeInBytes() const { return count() * sizeof(T); }
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