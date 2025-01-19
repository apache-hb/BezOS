#pragma once

#include "allocator/allocator.hpp"
#include "std/spinlock.hpp"

namespace mem {
    template<std::derived_from<mem::IAllocator> T>
    class SynchronizedAllocator : public T {
        stdx::SpinLock mLock;

    public:
        using T::T;

        void *allocate(size_t size) override {
            stdx::LockGuard _(mLock);
            return T::allocate(size);
        }

        void *allocateAligned(size_t size, size_t align) override {
            stdx::LockGuard _(mLock);
            return T::allocateAligned(size, align);
        }

        void deallocate(void *ptr, size_t size) override {
            stdx::LockGuard _(mLock);
            T::deallocate(ptr, size);
        }

        void *reallocate(void *old, size_t oldSize, size_t newSize) override {
            stdx::LockGuard _(mLock);
            return T::reallocate(old, oldSize, newSize);
        }
    };
}
