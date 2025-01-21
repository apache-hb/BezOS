#pragma once

#include "std/std.hpp"
#include "std/traits.hpp"

#include <algorithm>
#include <iterator>
#include <span>

namespace stdx {
    template<typename T, size_t N>
    class StaticVector {
        using SizeType = detail::ArraySize<N>;

        T mStorage[N];
        SizeType mSize;

    public:
        constexpr StaticVector()
            : mStorage()
            , mSize(0)
        { }

        constexpr StaticVector(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]])
            : StaticVector()
        {
            addRange(front, back);
        }

        constexpr StaticVector(std::span<const T> range)
            : StaticVector(range.data(), range.data() + range.size())
        { }

        constexpr StaticVector(std::initializer_list<T> list)
            : StaticVector(std::begin(list), std::end(list))
        { }

        constexpr StaticVector(const StaticVector& other)
            : StaticVector(std::begin(other), std::end(other))
        { }

        constexpr StaticVector& operator=(const StaticVector& other) {
            clear();
            addRange(other);
            return *this;
        }

        constexpr StaticVector(StaticVector&& other)
            : StaticVector(std::begin(other), std::end(other))
        {
            other.clear();
        }

        constexpr StaticVector& operator=(StaticVector&& other) {
            clear();
            addRange(other);
            other.clear();
            return *this;
        }

        constexpr bool operator==(const StaticVector& other) const {
            return std::equal(std::begin(*this), std::end(*this), std::begin(other), std::end(other));
        }

        constexpr size_t count() const { return mSize; }
        constexpr size_t capacity() const { return N; }
        constexpr size_t sizeInBytes() const { return count() * sizeof(T); }
        constexpr size_t available() const { return N - mSize; }
        constexpr bool isEmpty() const { return mSize == 0; }
        constexpr bool isFull() const { return mSize == N; }

        constexpr T *begin() { return mStorage; }
        constexpr T *end() { return begin() + mSize; }

        constexpr const T *begin() const { return mStorage; }
        constexpr const T *end() const { return begin() + mSize; }

        constexpr T *data() { return mStorage; }
        constexpr const T *data() const { return mStorage; }

        constexpr void clear() {
            std::destroy_n(begin(), count());
            mSize = 0;
        }

        constexpr bool add(T value) {
            if (isFull()) return false;

            std::construct_at(begin() + mSize, value);
            mSize += 1;
            return true;
        }

        constexpr size_t addRange(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) {
            size_t count = 0;
            while (front != back && !isFull()) {
                std::construct_at(begin() + mSize, *front++);
                mSize += 1;
                count++;
            }
            return count;
        }

        template<typename R> requires (IsRange<const T, R>)
        constexpr size_t addRange(const R& range) {
            return addRange(std::begin(range), std::end(range));
        }

        constexpr void pop() {
            if (!isEmpty()) {
                std::destroy_at(begin() + mSize - 1);
                mSize -= 1;
            }
        }

        constexpr void erase(const T& value) {
            for (size_t i = 0; i < count(); i++) {
                if (mStorage[i] == value) {
                    remove(i);
                    return;
                }
            }
        }

        constexpr void remove(size_t index) {
            if (index < count()) {
                std::destroy_at(mStorage + index);
                for (size_t i = index; i < count() - 1; i++)
                    mStorage[i] = mStorage[i + 1];
                std::destroy_at(mStorage + count() - 1);
                mSize -= 1;
            }
        }

        constexpr bool insert(size_t index, const T& value) {
            if (index > count() || isFull()) return false;

            for (size_t i = count(); i > index; i--)
                mStorage[i] = mStorage[i - 1];
            mStorage[index] = value;
            mSize += 1;
            return true;
        }

        constexpr T& operator[](size_t index) {
            TEST_ASSERT(index < count() && "Index out of bounds");
            return mStorage[index];
        }

        constexpr const T& operator[](size_t index) const {
            TEST_ASSERT(index < count() && "Index out of bounds");
            return mStorage[index];
        }

        constexpr T& get(size_t index) {
            TEST_ASSERT(index < count() && "Index out of bounds");
            return mStorage[index];
        }

        constexpr const T& get(size_t index) const {
            TEST_ASSERT(index < count() && "Index out of bounds");
            return mStorage[index];
        }

        constexpr T& front() {
            TEST_ASSERT(!isEmpty() && "Vector is empty");
            return mStorage[0];
        }

        constexpr const T& front() const {
            TEST_ASSERT(!isEmpty() && "Vector is empty");
            return mStorage[0];
        }

        constexpr T& back() {
            TEST_ASSERT(!isEmpty() && "Vector is empty");
            return mStorage[mSize - 1];
        }

        constexpr const T& back() const {
            TEST_ASSERT(!isEmpty() && "Vector is empty");
            return mStorage[mSize - 1];
        }
    };
}
