#pragma once

#include <memory>
#include <span>
#include <utility>

#include <stddef.h>

namespace stdx {
    template<typename T, typename Allocator>
    class Vector {
        [[no_unique_address]] Allocator mAllocator;

        T *mFront;
        T *mBack;
        T *mCapacity;

        void ensureExtra(size_t extra) {
            if (count() + extra > capacity()) {
                reserve(count() + extra);
            }
        }

    public:
        Vector(Allocator allocator)
            : mAllocator(allocator)
            , mFront(nullptr)
            , mBack(nullptr)
            , mCapacity(nullptr)
        { }

        Vector(Allocator allocator, size_t capacity)
            : mAllocator(allocator)
            , mFront(mAllocator.allocate(capacity))
            , mBack(mFront)
            , mCapacity(mFront + capacity)
        { }

        Vector(Allocator allocator, std::span<const T> src)
            : mAllocator(allocator)
            , mFront(mAllocator.allocate(src.size()))
            , mBack(mFront + src.size())
            , mCapacity(mFront + src.size())
        {
            std::uninitialized_copy(src.begin(), src.end(), mFront);
        }

        Vector(const Vector& other)
            : Vector(other.mAllocator, std::span(other))
        { }

        Vector& operator=(const Vector& other) {
            if (this != &other) {
                clear();
                mAllocator = other.mAllocator;
                mFront = mAllocator.allocate(other.count());
                mBack = mFront + other.count();
                mCapacity = mFront + other.count();

                std::uninitialized_copy(other.begin(), other.end(), mFront);
            }

            return *this;
        }

        Vector(Vector&& other)
            : mAllocator(other.mAllocator)
            , mFront(std::exchange(other.mFront, nullptr))
            , mBack(std::exchange(other.mBack, nullptr))
            , mCapacity(std::exchange(other.mCapacity, nullptr))
        { }

        Vector& operator=(Vector&& other) {
            if (this != &other) {
                clear();
                mAllocator = std::exchange(other.mAllocator, Allocator());
                mFront = std::exchange(other.mFront, nullptr);
                mBack = std::exchange(other.mBack, nullptr);
                mCapacity = std::exchange(other.mCapacity, nullptr);
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

        void reserveExact(size_t capacity) {
            size_t oldCapacity = this->capacity();
            if (capacity <= oldCapacity) {
                return;
            }

            T *newFront = (T*)mAllocator.allocate(capacity * sizeof(T), alignof(T));
            T *newBack = newFront + count();
            T *newCapacity = newFront + capacity;

            if (mFront) {
                std::uninitialized_move(mFront, mBack, newFront);
            }

            clear();

            if (mFront) {
                mAllocator.deallocate((void*)mFront, oldCapacity * sizeof(T));
            }

            mFront = newFront;
            mBack = newBack;
            mCapacity = newCapacity;
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
