#include <gtest/gtest.h>

#include "reentrant.hpp"
#include "system/schedule.hpp"

static void TestThread0(void *arg) {
    std::atomic<size_t> *counter = static_cast<std::atomic<size_t> *>(arg);
    *counter += 1;
}

static void TestThread1(void *arg) {
    std::atomic<size_t> *counter = static_cast<std::atomic<size_t> *>(arg);
    *counter += 1;
}

TEST(SchedulerTest, ContextSwitch) {
    sys::GlobalSchedule schedule;
    std::atomic<bool> done = false;
    std::atomic<size_t> signals = 0;
    std::atomic<size_t> switches = 0;
    std::unique_ptr<x64::page[]> stack0{ new x64::page[4] };
    std::unique_ptr<x64::page[]> stack1{ new x64::page[4] };
    std::atomic<size_t> counter0 = 0;
    std::atomic<size_t> counter1 = 0;
    schedule.initCpuSchedule(km::CpuCoreId(0), 128);

    pthread_t thread = ktest::CreateReentrantThread([&] {
        while (!done.load()) {

            switches += 1;
        }
    }, [&]([[maybe_unused]] siginfo_t *siginfo, [[maybe_unused]] mcontext_t *mc) {
        signals += 1;
    });

    sm::RcuSharedPtr<sys::Thread> thread0;
    sm::RcuSharedPtr<sys::Thread> thread1;

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::milliseconds(250);
    while (now < end) {
        ktest::AlertReentrantThread(thread);
        now = std::chrono::high_resolution_clock::now();
    }

    done.store(true);
    pthread_join(thread, nullptr);

    ASSERT_NE(signals.load(), 0);
    ASSERT_NE(switches.load(), 0);
    ASSERT_NE(counter0.load(), 0);
    ASSERT_NE(counter1.load(), 0);
}
