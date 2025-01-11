#pragma once

#include <memory>
#include <span>
#include <utility>

#include <stddef.h>

#include "allocator/allocator.hpp"
#include "kernel.hpp"

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

    public:
        ~Vector() {
            clear();
            if (mFront != nullptr) {
                mAllocator->deallocate(mFront, capacity());
            }
        }

        Vector(mem::IAllocator *allocator)
            : mAllocator(allocator)
            , mFront(nullptr)
            , mBack(nullptr)
            , mCapacity(nullptr)
        { }

        Vector(mem::IAllocator *allocator, size_t capacity)
            : mAllocator(allocator)
            , mFront(mAllocator->allocateArray<T>(capacity))
            , mBack(mFront)
            , mCapacity(mFront + capacity)
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
                clear();
                mAllocator = other.mAllocator;
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
                clear();
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
            for (T *it = mFront; it != mBack; it++) {
                std::destroy_at(it);
            }

            mBack = mFront;
        }

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
            } else {
                mFront = mAllocator->reallocateArray<T>(mFront, currentCount, oldCapacity, newCapacity);
                mBack = mFront + currentCount;
            }

            mCapacity = mFront + newCapacity;
        }

        void reserve(size_t newCapacity) {
            reserveExact(std::max(newCapacity, count() * 2));
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
    };
}
