#pragma once

#include <atomic>
#include <cstdint>

namespace stdx {
    class SharedSpinLock {
        static constexpr uint32_t kWriteLock = (1 << 31);

        std::atomic<uint32_t> mLock = 0;

    public:
        SharedSpinLock() = default;

        void lock() {
            uint32_t expected = 0;
            while (!mLock.compare_exchange_strong(expected, kWriteLock, std::memory_order_acquire)) {
                expected = 0;
            }
        }

        bool try_lock() {
            uint32_t expected = 0;
            return mLock.compare_exchange_strong(expected, kWriteLock, std::memory_order_acquire);
        }

        void unlock() {
            mLock.store(0, std::memory_order_release);
        }

        void lock_shared() {
            uint32_t expected;
            do {
                expected = mLock.load(std::memory_order_acquire) & ~kWriteLock;
            } while (!mLock.compare_exchange_strong(expected, expected + 1, std::memory_order_acquire));
        }

        bool try_lock_shared() {
            uint32_t expected = mLock.load(std::memory_order_acquire) & ~kWriteLock;
            return mLock.compare_exchange_strong(expected, expected + 1, std::memory_order_acquire);
        }

        void unlock_shared() {
            mLock.fetch_sub(1, std::memory_order_release);
        }
    };

    template<typename T>
    class UniqueLock {
        T& mLock;
    public:
        UniqueLock(T& lock)
            : mLock(lock)
        {
            mLock.lock();
        }

        ~UniqueLock() {
            mLock.unlock();
        }
    };

    template<typename T>
    class SharedLock {
        T& mLock;
    public:
        SharedLock(T& lock)
            : mLock(lock)
        {
            mLock.lock_shared();
        }

        ~SharedLock() {
            mLock.unlock_shared();
        }
    };
}
