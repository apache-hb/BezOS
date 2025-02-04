#pragma once

#include "std/traits.hpp"

#include <iterator>
#include <algorithm>
#include <array>

namespace stdx {
    template<typename T>
    class StringViewBase {
        static constexpr T kEmpty[] = { };
        const T *mFront;
        const T *mBack;

    public:
        constexpr StringViewBase()
            : StringViewBase(kEmpty, kEmpty)
        { }

        template<size_t N>
        constexpr StringViewBase(const T (&str)[N])
            : StringViewBase(std::begin(str), std::end(str))
        { }

        template<typename R> requires IsRange<const T, R>
        constexpr StringViewBase(const R& range)
            : StringViewBase(std::begin(range), std::end(range))
        { }

        constexpr StringViewBase(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]])
            : mFront(front)
            , mBack(back)
        { }

        template<size_t N>
        static constexpr StringViewBase ofString(const T (&str)[N]) {
            return StringViewBase(str, str + N - 1);
        }

        constexpr size_t count() const { return mBack - mFront; }
        constexpr size_t sizeInBytes() const { return count() * sizeof(T); }
        constexpr bool isEmpty() const { return mBack == mFront; }

        constexpr const T *begin() const { return mFront; }
        constexpr const T *end() const { return mBack; }

        constexpr const T& front() const { return *mFront; }
        constexpr const T& back() const { return *mBack; }

        constexpr StringViewBase substr(size_t first, size_t count) const {
            return StringViewBase(begin() + first, begin() + first + count);
        }

        constexpr StringViewBase substr(size_t count) const {
            return substr(0, count);
        }

        constexpr const T *data() const { return mFront; }

        constexpr const T& operator[](size_t index) const {
            return mFront[index];
        }

        friend constexpr auto operator<=>(StringViewBase lhs, StringViewBase rhs) {
            return std::lexicographical_compare_three_way(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
        }
    };

    using StringView = StringViewBase<char>;

    namespace literals {
        constexpr StringView operator""_sv(const char *str, size_t length) {
            return StringView(str, str + length);
        }
    }
}
