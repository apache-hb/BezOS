#pragma once

#include "allocator/allocator.hpp"
#include "std/spinlock.hpp"

namespace mem {
    template<std::derived_from<mem::IAllocator> T>
    class SynchronizedAllocator : public T {
        stdx::SpinLock mLock;

    public:
        using T::T;

        SynchronizedAllocator(T self)
            : T(std::move(self))
        { }

        void *allocate(size_t size) [[clang::allocating]] override {
            stdx::LockGuard guard(mLock);
            return T::allocate(size);
        }

        void *allocateAligned(size_t size, size_t align) [[clang::blocking, clang::allocating]] override {
            stdx::LockGuard guard(mLock);
            return T::allocateAligned(size, align);
        }

        void deallocate(void *ptr, size_t size) noexcept [[clang::blocking, clang::nonallocating]] override {
            stdx::LockGuard guard(mLock);
            T::deallocate(ptr, size);
        }

        void *reallocate(void *old, size_t oldSize, size_t newSize) [[clang::blocking, clang::allocating]] override {
            stdx::LockGuard guard(mLock);
            return T::reallocate(old, oldSize, newSize);
        }
    };
}
