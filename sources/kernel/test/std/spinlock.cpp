#include <gtest/gtest.h>

#include "common/compiler/compiler.hpp"

//
// Ignore thread safety analysis warnings for this test. We are purposefully
// violating preconditions to test the lock.
//
CLANG_DIAGNOSTIC_PUSH();
CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety-analysis");
CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

#include <shared_mutex>
#include <thread>

#include "processor.hpp"

#include "std/spinlock.hpp"
#include "std/recursive_mutex.hpp"
#include "std/shared_spinlock.hpp"

TEST(SpinLockTest, LockUnlock) {
    stdx::SpinLock lock;
    lock.lock();
    lock.unlock();
}

TEST(SpinLockTest, TryLockUnlock) {
    stdx::SpinLock lock;
    ASSERT_TRUE(lock.try_lock());
    lock.unlock();
}

TEST(SpinLockTest, TryLockTwice) {
    stdx::SpinLock lock;
    ASSERT_TRUE(lock.try_lock());
    ASSERT_FALSE(lock.try_lock());
    lock.unlock();
}

TEST(SpinLockTest, Excludes) {
    stdx::SpinLock lock;
    static constexpr size_t kSize = 0x1000;
    static constexpr size_t kIterations = 1000;
    std::unique_ptr<uint8_t[]> data(new uint8_t[kSize]);
    std::atomic<bool> error = false;

    {
        std::jthread t1([&] {
            for (size_t i = 0; i < kIterations; i++) {
                std::lock_guard guard(lock);
                uint8_t head = data[0];
                uint8_t tail = data[kSize - 1];
                if (head != tail) error = true;

                memset(data.get(), (i % UINT8_MAX), kSize);
            }
        });

        std::jthread t2([&] {
            for (size_t i = 0; i < kIterations; i++) {
                std::lock_guard guard(lock);
                uint8_t head = data[0];
                uint8_t tail = data[kSize - 1];
                if (head != tail) error = true;

                memset(data.get(), (i % UINT8_MAX), kSize);
            }
        });
    }

    ASSERT_FALSE(error);
}

TEST(SharedSpinLockTest, LockUnlock) {
    stdx::SharedSpinLock lock;
    lock.lock();
    lock.unlock();
}

TEST(SharedSpinLockTest, TryLockUnlock) {
    stdx::SharedSpinLock lock;
    ASSERT_TRUE(lock.try_lock());
    lock.unlock();
}

TEST(SharedSpinLockTest, TryLockTwice) {
    stdx::SharedSpinLock lock;
    ASSERT_TRUE(lock.try_lock());
    ASSERT_FALSE(lock.try_lock());
    lock.unlock();
}

TEST(SharedSpinLockTest, ManyReaders) {
    std::vector<std::jthread> threads;
    stdx::SharedSpinLock lock;
    static constexpr size_t kReaders = 10;
    static constexpr size_t kSize = 0x1000;
    static constexpr size_t kIterations = 1000;
    std::unique_ptr<uint8_t[]> data(new uint8_t[kSize]);
    std::atomic<bool> error = false;

    threads.emplace_back([&] {
        for (size_t i = 0; i < kIterations; i++) {
            std::unique_lock guard(lock);
            memset(data.get(), (i % UINT8_MAX), kSize);
        }
    });

    for (size_t i = 0; i < kReaders; i++) {
        threads.emplace_back([&] {
            for (size_t i = 0; i < kIterations; i++) {
                std::shared_lock guard(lock);
                uint8_t head = data[0];
                uint8_t tail = data[kSize - 1];
                if (head != tail) error = true;
            }
        });
    }

    threads.clear();

    ASSERT_FALSE(error);
}

TEST(SharedSpinLockTest, ManyWriters) {
    std::vector<std::jthread> threads;
    stdx::SharedSpinLock lock;
    static constexpr size_t kWriters = 10;
    static constexpr size_t kReaders = 10;
    static constexpr size_t kSize = 0x1000;
    static constexpr size_t kIterations = 1000;
    std::unique_ptr<uint8_t[]> data(new uint8_t[kSize]);
    std::atomic<bool> error = false;

    for (size_t i = 0; i < kWriters; i++) {
        threads.emplace_back([&] {
            for (size_t i = 0; i < kIterations; i++) {
                std::unique_lock guard(lock);
                memset(data.get(), (i % UINT8_MAX), kSize);
            }
        });
    }

    for (size_t i = 0; i < kReaders; i++) {
        threads.emplace_back([&] {
            for (size_t i = 0; i < kIterations; i++) {
                std::shared_lock guard(lock);
                uint8_t head = data[0];
                uint8_t tail = data[kSize - 1];
                if (head != tail) error = true;
            }
        });
    }

    threads.clear();

    ASSERT_FALSE(error);
}

km::CpuCoreId km::GetCurrentCoreId() {
    return CpuCoreId((uint32_t)(std::hash<std::thread::id>{}(std::this_thread::get_id()) % UINT32_MAX));
}

TEST(RecursiveSpinLockTest, Construct) {
    [[maybe_unused]]
    stdx::RecursiveSpinLock lock{};
}

TEST(RecursiveSpinLockTest, NestedLock) {
    stdx::RecursiveSpinLock lock{};

    lock.lock();
    lock.lock();
    lock.lock();

    lock.unlock();
    lock.unlock();
    lock.unlock();
}

TEST(RecursiveSpinLockTest, Update) {
    stdx::RecursiveSpinLock lock{};
    std::atomic<std::thread::id> id = std::this_thread::get_id();

    std::vector<std::jthread> threads;
    static constexpr size_t kWriters = 10;
    static constexpr size_t kIterations = 1000;
    std::atomic<bool> error = false;

    for (size_t i = 0; i < kWriters; i++) {
        threads.emplace_back([&] {
            for (size_t i = 0; i < kIterations; i++) {
                stdx::LockGuard guard(lock);
                id = std::this_thread::get_id();

                if (id != std::this_thread::get_id()) {
                    error = true;
                }

                if (rand() % 4) {
                    stdx::LockGuard guard2(lock);
                    if (id != std::this_thread::get_id()) {
                        error = true;
                    }
                }
            }
        });
    }

    threads.clear();
    ASSERT_FALSE(error);
}

CLANG_DIAGNOSTIC_POP();
