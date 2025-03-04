#pragma once

#include "std/traits.hpp"

#include "std/string_view.hpp"

#include <string.h>

namespace stdx {
    template<typename T, size_t N>
    class StaticStringBase {
        using SizeType = detail::ArraySize<N>;

        SizeType mSize;
        T mStorage[N];

        constexpr void init(const T *front, const T *back) {
            mSize = std::clamp<size_t>(back - front, 0, std::size(mStorage));
            std::copy_n(front, mSize, mStorage);
        }

    public:
        constexpr StaticStringBase()
            : mSize(0)
        { }

        constexpr StaticStringBase(T elem)
            : mSize(1)
            , mStorage({ elem })
        { }

        template<size_t S> requires (S <= N)
        constexpr StaticStringBase(const T (&str)[S])
            : StaticStringBase(str, str + S - 1)
        { }

        template<typename R> requires IsRange<const T, R>
        constexpr StaticStringBase(const R& range)
            : StaticStringBase(std::begin(range), std::end(range))
        { }

        constexpr StaticStringBase(std::initializer_list<T> list)
            : StaticStringBase(std::begin(list), std::end(list))
        { }

        constexpr StaticStringBase(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) {
            init(front, back);
        }

        constexpr size_t count() const { return mSize; }
        constexpr size_t capacity() const { return N; }
        constexpr size_t sizeInBytes() const { return mSize * sizeof(T); }

        constexpr bool isEmpty() const { return mSize == 0; }
        constexpr bool isFull() const { return mSize == N; }

        constexpr T *begin() { return mStorage; }
        constexpr T *end() { return mStorage + mSize; }

        constexpr const T *begin() const { return mStorage; }
        constexpr const T *end() const { return mStorage + mSize; }

        constexpr void clear() {
            mSize = 0;
        }

        template<typename R> requires IsRange<const T, R>
        constexpr void add(const R& range) {
            add(std::begin(range), std::end(range));
        }

        constexpr void add(T elem) {
            if (mSize < N) {
                mStorage[mSize++] = elem;
            }
        }

        constexpr void add(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) {
            size_t size = back - front;
            size_t newSize = mSize + size;
            if (newSize > N) {
                size = N - mSize;
            }

            std::copy_n(front, size, mStorage + mSize);
            mSize += size;
        }

        constexpr bool endsWith(StringViewBase<T> view) const {
            if (view.count() > mSize) {
                return false;
            }

            return std::equal(view.begin(), view.end(), end() - view.count());
        }

        constexpr bool startsWith(StringViewBase<T> view) const {
            if (view.count() > mSize) {
                return false;
            }

            return std::equal(view.begin(), view.end(), begin());
        }

        constexpr T& operator[](size_t index) {
            return mStorage[index];
        }

        constexpr const T& operator[](size_t index) const {
            return mStorage[index];
        }

        // TODO: this is annoying to use with string literals, it tries to compare the null terminator.
        template<typename R> requires IsRange<const T, R>
        constexpr bool operator==(const R& other) const {
            return std::equal(begin(), end(), std::begin(other), std::end(other));
        }
    };

    template<size_t N>
    using StaticString = StaticStringBase<char, N>;

    template<typename T>
    struct IsStaticString : std::false_type { };

    template<size_t N>
    struct IsStaticString<StaticString<N>> : std::true_type { };

    template<typename T>
    concept StaticStringType = IsStaticString<T>::value;

    static_assert(sizeof(StaticString<16>) == 17);
    static_assert(sizeof(StaticString<64>) == 65);
}
