#include <gtest/gtest.h>

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

class SchedulerTest : public testing::Test {
public:
    void SetUp() override {
        OsStatus status = task::SchedulerQueue::create(64, &queue);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    task::SchedulerQueue queue;
};

TEST_F(SchedulerTest, ContextSwitch) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;
    std::atomic<size_t> switches = 0;
    std::unique_ptr<x64::page[]> stack0{ new x64::page[4] };
    std::unique_ptr<x64::page[]> stack1{ new x64::page[4] };
    std::atomic<size_t> counter0 = 0;
    std::atomic<size_t> counter1 = 0;
    std::atomic<bool> done = false;

    auto [thread, handle] = ktest::CreateReentrantThreadPair([&] {
        while (!done.load()) {
            switches += 1;
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
            return;
        }

        signals += 1;

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
    });

    status = queue.enqueue({
        .registers = {
            .rdi = reinterpret_cast<uintptr_t>(&counter0),
            .rbp = reinterpret_cast<uintptr_t>(stack0.get() + 4) - 0x8,
            .rsp = reinterpret_cast<uintptr_t>(stack0.get() + 4) - 0x8,
            .rip = reinterpret_cast<uintptr_t>(TestThread0),
        }
    });
    ASSERT_EQ(status, OsStatusSuccess);

    status = queue.enqueue({
        .registers = {
            .rdi = reinterpret_cast<uintptr_t>(&counter1),
            .rbp = reinterpret_cast<uintptr_t>(stack1.get() + 4) - 0x8,
            .rsp = reinterpret_cast<uintptr_t>(stack1.get() + 4) - 0x8,
            .rip = reinterpret_cast<uintptr_t>(TestThread1),
        }
    });
    ASSERT_EQ(status, OsStatusSuccess);

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::milliseconds(250);
    while (now < end) {
        ktest::AlertReentrantThread(thread);
        now = std::chrono::high_resolution_clock::now();
    }

    done.store(true);
    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    ASSERT_NE(signals.load(), 0);
    ASSERT_NE(switches.load(), 0);
    ASSERT_NE(counter0.load(), 0);
    ASSERT_NE(counter1.load(), 0);
}
