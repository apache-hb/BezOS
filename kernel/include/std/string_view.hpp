#pragma once

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

        constexpr const T *data() const { return mFront; }

        constexpr const T& operator[](size_t index) const {
            return mFront[index];
        }

        constexpr bool operator==(StringViewBase other) const {
            return std::equal(begin(), end(), other.begin(), other.end());
        }
    };

    using StringView = StringViewBase<char>;

    namespace literals {
        constexpr StringView operator""_sv(const char *str, size_t length) {
            return StringView(str, str + length);
        }
    }
}
