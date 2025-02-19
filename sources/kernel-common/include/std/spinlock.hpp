#pragma once

#include <atomic>

#include <emmintrin.h>

namespace stdx {
    class SpinLock {
        std::atomic_flag mLock = ATOMIC_FLAG_INIT;

    public:
        void lock() {
            while (mLock.test_and_set(std::memory_order_acquire)) {
                _mm_pause();
            }
        }

        void unlock() {
            mLock.clear(std::memory_order_release);
        }

        bool try_lock() {
            return !mLock.test_and_set(std::memory_order_acquire);
        }
    };

    template<typename T>
    class [[nodiscard]] LockGuard {
        T& mLock;

    public:
        LockGuard(T& lock)
            : mLock(lock)
        {
            mLock.lock();
        }

        ~LockGuard() {
            mLock.unlock();
        }
    };
}
