#pragma once

#include "std/std.hpp"

namespace stdx {
    template<typename T>
    class StringViewBase {
        const T *mFront;
        const T *mBack;

    public:
        constexpr StringViewBase() noexcept
            : StringViewBase("")
        { }

        template<size_t N>
        constexpr StringViewBase(const T (&str)[N]) noexcept
            : StringViewBase(str, str + N - 1)
        { }

        constexpr StringViewBase(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) noexcept
            : mFront(front)
            , mBack(back)
        { }

        constexpr ssize_t count() const noexcept { return mBack - mFront; }
        constexpr ssize_t sizeInBytes() const noexcept { return count() * sizeof(T); }
        constexpr bool isEmpty() const noexcept { return mBack == mFront; }

        constexpr const T *begin() const noexcept { return mFront; }
        constexpr const T *end() const noexcept { return mBack; }

        constexpr const T *data() const noexcept { return mFront; }

        constexpr const T& operator[](size_t index) const noexcept {
            return mFront[index];
        }

        constexpr bool operator==(StringViewBase other) const noexcept {
            return other.count() == count() && memcmp(mFront, other.mFront, sizeInBytes()) == 0;
        }
    };

    using StringView = StringViewBase<char>;
}
