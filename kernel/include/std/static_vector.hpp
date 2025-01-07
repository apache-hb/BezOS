#pragma once

#include "std/std.hpp"
#include "std/traits.hpp"

#include <iterator>

namespace stdx {
    template<typename T>
    class StaticVectorBase {
        T *mFront;
        T *mBack;
        T *mCapacity;

    protected:
        constexpr StaticVectorBase(T *front, T *back, T *capacity)
            : mFront(front)
            , mBack(back)
            , mCapacity(capacity)
        { }

    public:
        constexpr size_t count() const { return mBack - mFront; }
        constexpr size_t capacity() const { return mCapacity - mFront; }
        constexpr size_t sizeInBytes() const { return count() * sizeof(T); }
        constexpr size_t available() const { return mCapacity - mBack; }
        constexpr bool isEmpty() const { return mBack == mFront; }
        constexpr bool isFull() const { return mBack == mCapacity; }

        constexpr T *begin() { return mFront; }
        constexpr T *end() { return mBack; }

        constexpr const T *begin() const { return mFront; }
        constexpr const T *end() const { return mBack; }

        constexpr void clear() { mBack = mFront; }

        constexpr bool add(const T& value) {
            if (isFull()) return false;

            std::construct_at(mBack++, value);
            return true;
        }

        constexpr size_t addRange(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]]) {
            size_t count = 0;
            while (front != back && !isFull()) {
                std::construct_at(mBack++, *front++);
                count++;
            }
            return count;
        }

        template<typename R> requires (IsRange<const T, R>)
        constexpr size_t addRange(const R& range) {
            return addRange(std::begin(range), std::end(range));
        }

        constexpr void pop() {
            if (!isEmpty()) std::destroy_at(--mBack);
        }

        constexpr void erase(const T& value) {
            for (size_t i = 0; i < count(); i++) {
                if (mFront[i] == value) {
                    remove(i);
                    return;
                }
            }
        }

        constexpr void remove(size_t index) {
            if (index < count()) {
                std::destroy_at(mFront + index);
                for (size_t i = index; i < count() - 1; i++)
                    mFront[i] = mFront[i + 1];
                std::destroy_at(--mBack);
            }
        }

        constexpr bool insert(size_t index, const T& value) {
            if (index > count() || isFull()) return false;

            for (size_t i = count(); i > index; i--)
                mFront[i] = mFront[i - 1];
            mFront[index] = value;
            mBack += 1;
            return true;
        }

        constexpr T& operator[](size_t index) {
            TEST_ASSERT(index < count() && "Index out of bounds");
            return mFront[index];
        }

        constexpr const T& operator[](size_t index) const {
            TEST_ASSERT(index < count() && "Index out of bounds");
            return mFront[index];
        }
    };

    template<typename T, size_t N>
    class StaticVector : public StaticVectorBase<T> {
        using Super = StaticVectorBase<T>;

        union { T mStorage[N]; };

    public:
        using Super::Super;

        constexpr StaticVector()
            : Super(std::begin(mStorage), std::begin(mStorage), std::end(mStorage))
        { }

        constexpr StaticVector(const T *front [[gnu::nonnull]], const T *back [[gnu::nonnull]])
            : StaticVector()
        {
            Super::addRange(front, back);
        }

        constexpr StaticVector(const StaticVector& other)
            : StaticVector()
        {
            Super::addRange(other);
        }

        constexpr StaticVector& operator=(const StaticVector& other) {
            Super::clear();
            Super::addRange(other);
            return *this;
        }
    };
}
