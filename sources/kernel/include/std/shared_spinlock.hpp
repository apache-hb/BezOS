#pragma once

#include <atomic>
#include <cstdint>

#include "std/mutex.h"

namespace stdx {
    class CAPABILITY("mutex") SharedSpinLock {
        static constexpr uint32_t kWriteLock = (1 << 31);

        std::atomic<uint32_t> mLock = 0;

    public:
        SharedSpinLock() = default;

        void lock() noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE() {
            uint32_t expected = 0;
            while (!mLock.compare_exchange_strong(expected, kWriteLock, std::memory_order_acquire)) {
                expected = 0;
            }
        }

        [[nodiscard]]
        bool try_lock() noexcept [[clang::nonblocking, clang::nonallocating]] TRY_ACQUIRE(true) {
            uint32_t expected = 0;
            return mLock.compare_exchange_strong(expected, kWriteLock, std::memory_order_acquire);
        }

        void unlock() noexcept [[clang::nonblocking, clang::nonallocating]] RELEASE() {
            mLock.store(0, std::memory_order_release);
        }

        void lock_shared() noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE_SHARED() {
            uint32_t expected;
            do {
                expected = mLock.load(std::memory_order_acquire) & ~kWriteLock;
            } while (!mLock.compare_exchange_strong(expected, expected + 1, std::memory_order_acquire));
        }

        [[nodiscard]]
        bool try_lock_shared() noexcept [[clang::nonblocking, clang::nonallocating]] TRY_ACQUIRE_SHARED(true) {
            uint32_t expected = mLock.load(std::memory_order_acquire) & ~kWriteLock;
            return mLock.compare_exchange_strong(expected, expected + 1, std::memory_order_acquire);
        }

        void unlock_shared() noexcept [[clang::nonblocking, clang::nonallocating]] RELEASE_SHARED() {
            mLock.fetch_sub(1, std::memory_order_release);
        }
    };

    template<typename T>
    class [[nodiscard, clang::scoped_lockable]] SCOPED_CAPABILITY UniqueLock {
        T& mLock;
    public:
        UniqueLock(T& lock) noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE(lock)
            : mLock(lock)
        {
            mLock.lock();
        }

        ~UniqueLock() noexcept [[clang::nonblocking, clang::nonallocating]] RELEASE() {
            mLock.unlock();
        }
    };

    template<typename T>
    class [[nodiscard, clang::scoped_lockable]] SCOPED_CAPABILITY SharedLock {
        T& mLock;
    public:
        SharedLock(T& lock) noexcept [[clang::blocking, clang::nonallocating]] ACQUIRE_SHARED(lock)
            : mLock(lock)
        {
            mLock.lock_shared();
        }

        ~SharedLock() noexcept [[clang::nonblocking, clang::nonallocating]] RELEASE() {
            mLock.unlock_shared();
        }
    };
}
