#pragma once

#include <atomic>
#include <cstdint>

namespace stdx {
    class [[clang::capability("mutex")]] SharedSpinLock {
        static constexpr uint32_t kWriteLock = (1 << 31);

        std::atomic<uint32_t> mLock = 0;

    public:
        SharedSpinLock() = default;

        [[clang::acquire_capability]]
        void lock() {
            uint32_t expected = 0;
            while (!mLock.compare_exchange_strong(expected, kWriteLock, std::memory_order_acquire)) {
                expected = 0;
            }
        }

        [[nodiscard, clang::try_acquire_capability(true)]]
        bool try_lock() {
            uint32_t expected = 0;
            return mLock.compare_exchange_strong(expected, kWriteLock, std::memory_order_acquire);
        }

        [[clang::release_capability]]
        void unlock() {
            mLock.store(0, std::memory_order_release);
        }

        [[clang::acquire_shared_capability]]
        void lock_shared() {
            uint32_t expected;
            do {
                expected = mLock.load(std::memory_order_acquire) & ~kWriteLock;
            } while (!mLock.compare_exchange_strong(expected, expected + 1, std::memory_order_acquire));
        }

        [[nodiscard, clang::try_acquire_shared_capability(true)]]
        bool try_lock_shared() {
            uint32_t expected = mLock.load(std::memory_order_acquire) & ~kWriteLock;
            return mLock.compare_exchange_strong(expected, expected + 1, std::memory_order_acquire);
        }

        [[clang::release_shared_capability]]
        void unlock_shared() {
            mLock.fetch_sub(1, std::memory_order_release);
        }
    };

    template<typename T>
    class [[nodiscard, clang::scoped_lockable]] UniqueLock {
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
    class [[nodiscard, clang::scoped_lockable]] SharedLock {
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
