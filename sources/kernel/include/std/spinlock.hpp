#pragma once

#include <atomic>

#include <emmintrin.h>

#include "std/mutex.h"

namespace stdx {
    class CAPABILITY("mutex") SpinLock {
        std::atomic_flag mLock = ATOMIC_FLAG_INIT;

    public:
        void lock() [[clang::blocking]] ACQUIRE() {
            while (mLock.test_and_set(std::memory_order_acquire)) {
                _mm_pause();
            }
        }

        void unlock() RELEASE() {
            mLock.clear(std::memory_order_release);
        }

        [[nodiscard]]
        bool try_lock() TRY_ACQUIRE(true) {
            return !mLock.test_and_set(std::memory_order_acquire);
        }
    };

    template<typename T>
    class [[nodiscard, clang::scoped_lockable]] LockGuard {
        T& mLock;

    public:
        LockGuard(T& lock) [[clang::blocking]] ACQUIRE(lock)
            : mLock(lock)
        {
            mLock.lock();
        }

        ~LockGuard() RELEASE() {
            mLock.unlock();
        }
    };
}
