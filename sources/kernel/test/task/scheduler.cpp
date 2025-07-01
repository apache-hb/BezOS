#include <gtest/gtest.h>
#include <thread>

#include "reentrant.hpp"
#include "task/scheduler.hpp"

[[gnu::target("general-regs-only")]]
static void TestThread0(void *arg) {
    std::atomic<size_t> *counter = static_cast<std::atomic<size_t> *>(arg);
    while (true) {
        *counter += 1;
    }
}

[[gnu::target("general-regs-only")]]
static void TestThread1(void *arg) {
    std::atomic<size_t> *counter = static_cast<std::atomic<size_t> *>(arg);
    while (true) {
        *counter += 1;
    }
}

static bool tryContextSwitch(task::SchedulerQueue& queue, mcontext_t *mc) {
    task::TaskState state {
        .registers = {
            .rax = static_cast<uintptr_t>(mc->gregs[REG_RAX]),
            .rbx = static_cast<uintptr_t>(mc->gregs[REG_RBX]),
            .rcx = static_cast<uintptr_t>(mc->gregs[REG_RCX]),
            .rdx = static_cast<uintptr_t>(mc->gregs[REG_RDX]),
            .rdi = static_cast<uintptr_t>(mc->gregs[REG_RDI]),
            .rsi = static_cast<uintptr_t>(mc->gregs[REG_RSI]),
            .r8 = static_cast<uintptr_t>(mc->gregs[REG_R8]),
            .r9 = static_cast<uintptr_t>(mc->gregs[REG_R9]),
            .r10 = static_cast<uintptr_t>(mc->gregs[REG_R10]),
            .r11 = static_cast<uintptr_t>(mc->gregs[REG_R11]),
            .r12 = static_cast<uintptr_t>(mc->gregs[REG_R12]),
            .r13 = static_cast<uintptr_t>(mc->gregs[REG_R13]),
            .r14 = static_cast<uintptr_t>(mc->gregs[REG_R14]),
            .r15 = static_cast<uintptr_t>(mc->gregs[REG_R15]),
            .rbp = static_cast<uintptr_t>(mc->gregs[REG_RBP]),
            .rsp = static_cast<uintptr_t>(mc->gregs[REG_RSP]),
            .rip = static_cast<uintptr_t>(mc->gregs[REG_RIP]),
            .rflags = static_cast<uintptr_t>(mc->gregs[REG_EFL]),
            .cs = static_cast<uintptr_t>((mc->gregs[REG_CSGSFS] >> 0) & 0xFFFF),
        }
    };

    if (queue.reschedule(&state) == task::ScheduleResult::eIdle) {
        return false;
    }

    mc->gregs[REG_RAX] = static_cast<greg_t>(state.registers.rax);
    mc->gregs[REG_RBX] = static_cast<greg_t>(state.registers.rbx);
    mc->gregs[REG_RCX] = static_cast<greg_t>(state.registers.rcx);
    mc->gregs[REG_RDX] = static_cast<greg_t>(state.registers.rdx);
    mc->gregs[REG_RDI] = static_cast<greg_t>(state.registers.rdi);
    mc->gregs[REG_RSI] = static_cast<greg_t>(state.registers.rsi);
    mc->gregs[REG_R8] = static_cast<greg_t>(state.registers.r8);
    mc->gregs[REG_R9] = static_cast<greg_t>(state.registers.r9);
    mc->gregs[REG_R10] = static_cast<greg_t>(state.registers.r10);
    mc->gregs[REG_R11] = static_cast<greg_t>(state.registers.r11);
    mc->gregs[REG_R12] = static_cast<greg_t>(state.registers.r12);
    mc->gregs[REG_R13] = static_cast<greg_t>(state.registers.r13);
    mc->gregs[REG_R14] = static_cast<greg_t>(state.registers.r14);
    mc->gregs[REG_R15] = static_cast<greg_t>(state.registers.r15);
    mc->gregs[REG_RBP] = static_cast<greg_t>(state.registers.rbp);
    mc->gregs[REG_RSP] = static_cast<greg_t>(state.registers.rsp);
    mc->gregs[REG_RIP] = static_cast<greg_t>(state.registers.rip);
    mc->gregs[REG_EFL] = static_cast<greg_t>(state.registers.rflags);

    return true;
}

class SchedulerTest : public testing::Test {
public:
    void SetUp() override {
        OsStatus status = task::SchedulerQueue::create(64, &queue);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    task::SchedulerQueue queue;

    OsStatus enqueueCounterTask(std::atomic<size_t> *counter, x64::page *stack, void (*task)(void *), task::SchedulerEntry *entry) {
        OsStatus status = queue.enqueue({
            .registers = {
                .rdi = reinterpret_cast<uintptr_t>(counter),
                .rbp = reinterpret_cast<uintptr_t>(stack + 4) - 0x8,
                .rsp = reinterpret_cast<uintptr_t>(stack + 4) - 0x8,
                .rip = reinterpret_cast<uintptr_t>(task),
            }
        }, km::StackMappingAllocation{}, km::StackMappingAllocation{}, entry);
        return status;
    }

    template<typename F>
    void runQueue(pthread_t thread, std::chrono::milliseconds duration, F&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        auto now = start;
        auto end = start + duration;
        while (now < end) {
            ktest::AlertReentrantThread(thread);
            auto newTime = std::chrono::high_resolution_clock::now();
            auto elapsed = newTime - start;
            now = newTime;
            func(elapsed);
        }
    }

    void runQueue(pthread_t thread, std::chrono::milliseconds duration) {
        runQueue(thread, duration, [](auto) {});
    }
};

TEST_F(SchedulerTest, ContextSwitch) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;
    std::unique_ptr<x64::page[]> stack0{ new x64::page[4] };
    std::unique_ptr<x64::page[]> stack1{ new x64::page[4] };
    std::atomic<size_t> counter0 = 0;
    std::atomic<size_t> counter1 = 0;
    std::atomic<bool> done = false;

    auto [thread, handle] = ktest::CreateReentrantThreadPair([&] {
        while (!done.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, ktest::kReentrantSignal);
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    }, [&]([[maybe_unused]] siginfo_t *siginfo, mcontext_t *mc) {
        if (done.load()) {
            pthread_exit(nullptr);
            return;
        }

        if (!tryContextSwitch(queue, mc)) {
            return; // No task to switch to
        }

        signals += 1;
    });

    task::SchedulerEntry task0;
    task::SchedulerEntry task1;

    status = enqueueCounterTask(&counter0, stack0.get(), TestThread0, &task0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = enqueueCounterTask(&counter1, stack1.get(), TestThread1, &task1);
    ASSERT_EQ(status, OsStatusSuccess);

    runQueue(thread, std::chrono::milliseconds(250));
    done.store(true);

    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    EXPECT_NE(signals.load(), 0);
    EXPECT_NE(counter0.load(), 0);
    EXPECT_NE(counter1.load(), 0);
}

TEST_F(SchedulerTest, Terminate) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;
    std::unique_ptr<x64::page[]> stack0{ new x64::page[4] };
    std::unique_ptr<x64::page[]> stack1{ new x64::page[4] };
    std::atomic<size_t> counter0 = 0;
    std::atomic<size_t> counter1 = 0;
    std::atomic<bool> done = false;

    std::atomic<size_t> savedCounter0 = 0;

    auto [thread, handle] = ktest::CreateReentrantThreadPair([&] {
        while (!done.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, ktest::kReentrantSignal);
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    }, [&]([[maybe_unused]] siginfo_t *siginfo, mcontext_t *mc) {
        if (done.load()) {
            pthread_exit(nullptr);
            return;
        }

        if (!tryContextSwitch(queue, mc)) {
            return; // No task to switch to
        }

        signals += 1;
    });

    task::SchedulerEntry task0;
    task::SchedulerEntry task1;

    status = enqueueCounterTask(&counter0, stack0.get(), TestThread0, &task0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = enqueueCounterTask(&counter1, stack1.get(), TestThread1, &task1);
    ASSERT_EQ(status, OsStatusSuccess);

    runQueue(thread, std::chrono::milliseconds(250), [&](auto elapsed) {
        if (elapsed > std::chrono::milliseconds(100)) {
            task0.terminate();
        }

        // Once the task is terminated, it should not be rescheduled.
        if (task0.isClosed()) {
            if (savedCounter0.load() == 0) {
                savedCounter0.store(counter0.load());
            } else {
                ASSERT_EQ(savedCounter0.load(), counter0.load());
            }
        }
    });

    done.store(true);

    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    EXPECT_NE(signals.load(), 0);
    EXPECT_NE(counter0.load(), 0);
    EXPECT_NE(counter1.load(), 0);
    EXPECT_NE(savedCounter0.load(), 0);
    EXPECT_EQ(counter0.load(), savedCounter0.load());
    EXPECT_TRUE(task0.isClosed());
}
