#pragma once

#include <atomic>

#include <emmintrin.h>

#include "std/mutex.h"
#include "std/funqual.hpp"

namespace stdx {
    class CAPABILITY("mutex") SpinLock {
        std::atomic_flag mLock = ATOMIC_FLAG_INIT;

    public:
        void lock() noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE() NON_REENTRANT {
            while (mLock.test_and_set(std::memory_order_acquire)) {
                _mm_pause();
            }
        }

        void unlock() noexcept [[clang::nonblocking, clang::nonallocating]] RELEASE() NON_REENTRANT {
            mLock.clear(std::memory_order_release);
        }

        [[nodiscard]]
        bool try_lock() noexcept [[clang::nonblocking, clang::nonallocating]] TRY_ACQUIRE(true) NON_REENTRANT {
            return !mLock.test_and_set(std::memory_order_acquire);
        }
    };

    template<typename T>
    class [[nodiscard, clang::scoped_lockable]] LockGuard {
        T& mLock;

    public:
        LockGuard(T& lock) noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE(lock) NON_REENTRANT
            : mLock(lock)
        {
            mLock.lock();
        }

        ~LockGuard() noexcept [[clang::nonblocking]] RELEASE() NON_REENTRANT {
            mLock.unlock();
        }
    };
}
