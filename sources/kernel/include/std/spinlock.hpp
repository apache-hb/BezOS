#pragma once

#include <atomic>

#include <emmintrin.h>

#include "std/mutex.h"

namespace stdx {
    class CAPABILITY("mutex") SpinLock {
        std::atomic_flag mLock = ATOMIC_FLAG_INIT;

    public:
        void lock() noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE() {
            while (mLock.test_and_set(std::memory_order_acquire)) {
                _mm_pause();
            }
        }

        void unlock() noexcept [[clang::nonblocking, clang::nonallocating]] RELEASE() {
            mLock.clear(std::memory_order_release);
        }

        [[nodiscard]]
        bool try_lock() noexcept [[clang::nonblocking, clang::nonallocating]] TRY_ACQUIRE(true) {
            return !mLock.test_and_set(std::memory_order_acquire);
        }
    };

    template<typename T>
    class SCOPED_CAPABILITY [[nodiscard]] LockGuard {
        T& mLock;

    public:
        LockGuard(T& lock) noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE(lock)
            : mLock(lock)
        {
            mLock.lock();
        }

        ~LockGuard() noexcept [[clang::nonblocking]] RELEASE() {
            mLock.unlock();
        }
    };
}
