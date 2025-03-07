#pragma once

#include <atomic>

#include <emmintrin.h>
#include <tuple>

namespace stdx {
    class [[clang::capability("mutex")]] SpinLock {
        std::atomic_flag mLock = ATOMIC_FLAG_INIT;

    public:
        [[clang::acquire_capability]]
        void lock() [[clang::blocking]] {
            while (mLock.test_and_set(std::memory_order_acquire)) {
                _mm_pause();
            }
        }

        [[clang::release_capability]]
        void unlock() {
            mLock.clear(std::memory_order_release);
        }

        [[nodiscard, clang::try_acquire_capability(true)]]
        bool try_lock() {
            return !mLock.test_and_set(std::memory_order_acquire);
        }
    };

    template<typename T>
    class [[nodiscard, clang::scoped_lockable]] LockGuard {
        T& mLock;

    public:
        LockGuard(T& lock) [[clang::blocking]] __attribute__((acquire_capability(lock)))
            : mLock(lock)
        {
            mLock.lock();
        }

        ~LockGuard() __attribute__((release_capability)) {
            mLock.unlock();
        }
    };
}
