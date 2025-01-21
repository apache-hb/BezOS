#pragma once

#include <memory>
#include <span>

#include <stddef.h>

#include "allocator/allocator.hpp"
#include "panic.hpp"

namespace stdx {
    template<typename T>
    class Vector {
        mem::IAllocator *mAllocator;

        T *mFront;
        T *mBack;
        T *mCapacity;

        void ensureExtra(size_t extra) {
            if ((count() + extra) >= capacity()) {
                reserveExact(capacity() + extra);
            }
        }

        void releaseMemory() {
            if (mFront != nullptr) {
                std::destroy_n(mFront, count());
                mAllocator->deallocate(mFront, capacity());
            }

            mFront = nullptr;
            mBack = nullptr;
            mCapacity = nullptr;
        }

    public:
        ~Vector() {
            releaseMemory();
        }

        Vector(mem::IAllocator *allocator)
            : Vector(allocator, 1)
        { }

        Vector(mem::IAllocator *allocator, size_t capacity)
            : mAllocator(allocator)
            , mFront(mAllocator->allocateArray<T>(std::max<size_t>(capacity, 1)))
            , mBack(mFront)
            , mCapacity(mFront + std::max<size_t>(capacity, 1))
        {
            KM_CHECK(mFront != nullptr, "Failed to allocate vector buffer");
        }

        Vector(mem::IAllocator *allocator, std::span<const T> src)
            : Vector(allocator, src.size())
        {
            KM_CHECK(mFront != nullptr, "Failed to allocate vector buffer");
            addRange(src);
        }

        Vector(const Vector& other)
            : Vector(other.mAllocator, std::span(other))
        { }

        Vector& operator=(const Vector& other) {
            if (this != &other) {
                releaseMemory();
                mAllocator = other.mAllocator;
                mFront = mAllocator->allocateArray<T>(other.count());
                mBack = mFront;
                mCapacity = mFront + other.count();
                addRange(other);
            }

            return *this;
        }

        Vector(Vector&& other)
            : mAllocator(other.mAllocator)
            , mFront(other.mFront)
            , mBack(other.mBack)
            , mCapacity(other.mCapacity)
        {
            other.mFront = nullptr;
            other.mBack = nullptr;
            other.mCapacity = nullptr;
        }

        Vector& operator=(Vector&& other) {
            if (this != &other) {
                releaseMemory();
                mAllocator = other.mAllocator;
                mFront = other.mFront;
                mBack = other.mBack;
                mCapacity = other.mCapacity;

                other.mFront = nullptr;
                other.mBack = nullptr;
                other.mCapacity = nullptr;
            }

            return *this;
        }

        size_t count() const { return mBack - mFront; }
        size_t capacity() const { return mCapacity - mFront; }
        bool isEmpty() const { return mBack == mFront; }

        void clear() {
            std::destroy_n(mFront, count());
            mBack = mFront;
        }

        T *data() { return mFront; }

        T *begin() { return mFront; }
        T *end() { return mBack; }

        const T *begin() const { return mFront; }
        const T *end() const { return mBack; }

        T& operator[](size_t index) {
            return mFront[index];
        }

        const T& operator[](size_t index) const {
            return mFront[index];
        }

        T& front() { return *mFront; }
        const T& front() const { return *mFront; }

        T& back() { return *(mBack - 1); }
        const T& back() const { return *(mBack - 1); }

        T pop() { return *--mBack; }

        void reserveExact(size_t newCapacity) {
            size_t oldCapacity = capacity();
            if (newCapacity <= oldCapacity) {
                return;
            }

            // need to cache this here for correctness
            size_t currentCount = count();

            if (mFront == nullptr) {
                mFront = mAllocator->allocateArray<T>(newCapacity);
                mBack = mFront;
                mCapacity = mFront + newCapacity;
            } else {
                mFront = mAllocator->reallocateArray<T>(mFront, currentCount, oldCapacity, newCapacity);
                mBack = mFront + currentCount;
                mCapacity = mFront + newCapacity;
            }
        }

        void reserve(size_t newCapacity) {
            reserveExact(std::max(newCapacity, count() * 2));
        }

        void resize(size_t newSize) {
            if (newSize < count()) {
                std::destroy_n(mFront + newSize, count() - newSize);
                mBack = mFront + newSize;
            } else if (newSize > count()) {
                ensureExtra(newSize - count());
                std::uninitialized_default_construct(mBack, mFront + newSize);
                mBack = mFront + newSize;
            }
        }

        void add(const T& value) {
            ensureExtra(1);

            std::construct_at(mBack++, value);
        }

        void addRange(std::span<const T> src) {
            ensureExtra(src.size());

            std::uninitialized_copy(src.begin(), src.end(), mBack);
            mBack += src.size();
        }

        void remove(size_t index) {
            std::destroy_at(mFront + index);
            std::copy(mFront + index + 1, mBack, mFront + index);
            std::destroy_at(--mBack);
        }
    };
}
