#pragma once

#include "std/std.hpp"

#include <iterator>

namespace stdx {
    template<typename T>
    class StringViewBase {
        const T *mFront;
        const T *mBack;

    public:
        constexpr StringViewBase()
            : StringViewBase("")
        { }

        template<size_t N>
        constexpr StringViewBase(const T (&str)[N])
            : StringViewBase(std::begin(str), std::end(str))
        { }

        constexpr StringViewBase(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]])
            : mFront(front)
            , mBack(back)
        { }

        constexpr ssize_t count() const { return mBack - mFront; }
        constexpr ssize_t sizeInBytes() const { return count() * sizeof(T); }
        constexpr bool isEmpty() const { return mBack == mFront; }

        constexpr const T *begin() const { return mFront; }
        constexpr const T *end() const { return mBack; }

        constexpr const T *data() const { return mFront; }

        constexpr const T& operator[](size_t index) const {
            return mFront[index];
        }

        constexpr bool operator==(StringViewBase other) const {
            return other.count() == count() && memcmp(mFront, other.mFront, sizeInBytes()) == 0;
        }
    };

    using StringView = StringViewBase<char>;
}
