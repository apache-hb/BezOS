#include <gtest/gtest.h>
#include <thread>

#include "reentrant.hpp"
#include "task/scheduler_queue.hpp"

/// lifted from llvm compiler-rt
[[gnu::target("general-regs-only")]]
size_t ucontextSize(void *ctx) {
    // Added in Linux kernel 3.4.0, merged to glibc in 2.16
#ifndef FP_XSTATE_MAGIC1
#   define FP_XSTATE_MAGIC1 0x46505853U
#endif
    // See kernel arch/x86/kernel/fpu/signal.c for details.
    const auto *fpregs = static_cast<ucontext_t *>(ctx)->uc_mcontext.fpregs;
    // The member names differ across header versions, but the actual layout
    // is always the same.  So avoid using members, just use arithmetic.
    const uint32_t *after_xmm = reinterpret_cast<const uint32_t *>(fpregs + 1) - 24;
    if (after_xmm[12] == FP_XSTATE_MAGIC1)
        return reinterpret_cast<const char *>(fpregs) + after_xmm[13] -
            static_cast<const char *>(ctx);

    return sizeof(ucontext_t);
}

static task::SchedulerEntry *gCurrentEntry = nullptr;

struct ThreadTestState {
    std::unique_ptr<x64::page[]> stackStorage;
    std::unique_ptr<x64::page[]> xsaveStorage;
    std::atomic<bool> running{true};
    std::atomic<size_t> counter{0};
    task::SchedulerEntry *entry;
    task::SchedulerQueue *queue;
};

static void initTestState(ThreadTestState *state) {
    state->stackStorage = std::make_unique<x64::page[]>(4);
    // Use a big number of pages to ensure we have enough space for the xsave state
    // Linux has no easy way to determine the size required to save the fpu state.
    state->xsaveStorage = std::make_unique<x64::page[]>(4);
    state->entry = nullptr;
    state->queue = nullptr;
}

static ThreadTestState newTestState() {
    return ThreadTestState {
        .stackStorage = std::make_unique<x64::page[]>(4),
        .xsaveStorage = std::make_unique<x64::page[]>(4),
        .entry = nullptr,
        .queue = nullptr,
    };
}

[[gnu::target("general-regs-only")]]
static void TestThread0(void *arg) {
    ThreadTestState *state = static_cast<ThreadTestState *>(arg);
    task::SchedulerQueue *queue = state->queue;
    task::SchedulerEntry *entry = state->entry;
    // printf("TestThread0: %p\n", (void*)entry);
    while (true) {
        state->counter += 1;

        if (queue->getCurrentTask() != entry) {
            state->counter.store(0x12345678);
            break;
        }

        // ASSERT_EQ(queue->getCurrentTask(), entry);
    }

    while (true) {

    }
}

[[gnu::target("general-regs-only")]]
static void TestThread1(void *arg) {
    ThreadTestState *state = static_cast<ThreadTestState *>(arg);
    task::SchedulerQueue *queue = state->queue;
    task::SchedulerEntry *entry = state->entry;
    // printf("TestThread1: %p\n", (void*)entry);
    while (true) {
        state->counter += 1;

        if (queue->getCurrentTask() != entry) {
            state->counter.store(0x12345678);
            break;
        }

        // ASSERT_EQ(queue->getCurrentTask(), entry);
    }

    while (true) {

    }
}

[[gnu::target("general-regs-only"), gnu::naked]]
static void IdleThread() {
    asm("1:\n jmp 1b");
}

static bool tryContextSwitch(task::SchedulerQueue& queue, ucontext_t *uc) {
    mcontext_t *mc = &uc->uc_mcontext;
    x64::XSave *xsave = nullptr;
#if 0
    size_t ucSize = ucontextSize(uc) - offsetof(ucontext_t, uc_mcontext.fpregs);
    printf("uc: %zu, %zu\n", ucontextSize(uc), ucSize);
    if (task::SchedulerEntry *currentTask = queue.getCurrentTask()) {
        task::TaskState& taskState = currentTask->getState();
        xsave = taskState.xsave;
        memcpy(xsave, &uc->uc_mcontext.fpregs, ucSize);
        printf("Copied task fpu state\n");
    }
#endif
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
        },
        .xsave = xsave,
    };

    if (queue.reschedule(&state) == task::ScheduleResult::eIdle) {
        mc->gregs[REG_RIP] = reinterpret_cast<greg_t>(IdleThread);
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

#if 0
    printf("Copying new fpu state %p\n", (void*)state.xsave);
    if (state.xsave != nullptr) {
        memcpy(&uc->uc_mcontext.fpregs, state.xsave, ucSize);
    }

    printf("Copied new fpu state\n");
#endif
    return true;
}

class SchedulerTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
    }

    void SetUp() override {
        gCurrentEntry = nullptr;
        OsStatus status = task::SchedulerQueue::create(64, &queue);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    task::SchedulerQueue queue;

    OsStatus enqueueCounterTask(ThreadTestState *state, void (*task)(void *), task::SchedulerEntry *entry) {
        state->entry = entry;
        state->queue = &queue;
        x64::page *stack = state->stackStorage.get();
        x64::page *xsave = state->xsaveStorage.get();

        OsStatus status = queue.enqueue({
            .registers = {
                .rdi = reinterpret_cast<uintptr_t>(state),
                .rbp = reinterpret_cast<uintptr_t>(stack + 4) - 0x8,
                .rsp = reinterpret_cast<uintptr_t>(stack + 4) - 0x8,
                .rip = reinterpret_cast<uintptr_t>(task),
            },
            .xsave = (x64::XSave*)(xsave),
        }, entry);
        return status;
    }

    template<typename F>
    void runQueue(pthread_t thread, F&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        auto now = start;
        while (true) {
            ktest::AlertReentrantThread(thread);
            auto newTime = std::chrono::high_resolution_clock::now();
            auto elapsed = newTime - start;
            now = newTime;
            if (func(elapsed)) {
                break;
            }
        }
    }

    void runQueue(pthread_t thread, std::chrono::milliseconds duration) {
        runQueue(thread, [duration](auto elapsed) -> bool {
            return elapsed > duration;
        });
    }
};

TEST_F(SchedulerTest, ContextSwitch) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;

    ThreadTestState state0 = newTestState();
    ThreadTestState state1 = newTestState();
    std::atomic<bool> done = false;

    auto [thread, handle] = ktest::CreateReentrantThreadPair([&] {
        while (!done.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, ktest::kReentrantSignal);
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    }, [&]([[maybe_unused]] siginfo_t *siginfo, ucontext_t *uc) {
        if (done.load()) {
            pthread_exit(nullptr);
            return;
        }

        if (!tryContextSwitch(queue, uc)) {
            return; // No task to switch to
        }

        // printf("Context switch done\n");

        signals += 1;
    });

    task::SchedulerEntry task0;
    task::SchedulerEntry task1;

    status = enqueueCounterTask(&state0, TestThread0, &task0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = enqueueCounterTask(&state1, TestThread1, &task1);
    ASSERT_EQ(status, OsStatusSuccess);

    runQueue(thread, [&](auto elapsed) {
        return state0.counter.load() != 0 && state1.counter.load() != 0 &&
               elapsed > std::chrono::milliseconds(100);
    });

    done.store(true);

    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    EXPECT_NE(signals.load(), 0);
    EXPECT_NE(state0.counter.load(), 0);
    EXPECT_NE(state1.counter.load(), 0);
}

TEST_F(SchedulerTest, Terminate) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;

    ThreadTestState state0 = newTestState();
    ThreadTestState state1 = newTestState();
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
    }, [&]([[maybe_unused]] siginfo_t *siginfo, ucontext_t *uc) {
        if (done.load()) {
            pthread_exit(nullptr);
            return;
        }

        if (!tryContextSwitch(queue, uc)) {
            return; // No task to switch to
        }

        signals += 1;
    });

    task::SchedulerEntry task0;
    task::SchedulerEntry task1;

    status = enqueueCounterTask(&state0, TestThread0, &task0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = enqueueCounterTask(&state1, TestThread1, &task1);
    ASSERT_EQ(status, OsStatusSuccess);

    runQueue(thread, [&](auto elapsed) {
        if (state0.counter.load() == 0 || state1.counter.load() == 0) {
            return false;
        }

        if (elapsed > std::chrono::milliseconds(100)) {
            task0.terminate();
        }

        // Once the task is terminated, it should not be rescheduled.
        if (task0.isClosed()) {
            if (savedCounter0.load() == 0) {
                savedCounter0.store(state0.counter.load());
            } else {
                EXPECT_EQ(savedCounter0.load(), state0.counter.load());
            }
        }

        return task0.isClosed() && elapsed > std::chrono::milliseconds(250);
    });

    done.store(true);

    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    EXPECT_NE(signals.load(), 0);
    EXPECT_NE(state0.counter.load(), 0);
    EXPECT_NE(state1.counter.load(), 0);
    EXPECT_NE(savedCounter0.load(), 0);
    EXPECT_EQ(state0.counter.load(), savedCounter0.load());
    EXPECT_TRUE(task0.isClosed());
}

TEST_F(SchedulerTest, ExhaustThreads) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;

    ThreadTestState state0 = newTestState();
    ThreadTestState state1 = newTestState();
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
    }, [&]([[maybe_unused]] siginfo_t *siginfo, ucontext_t *uc) {
        if (done.load()) {
            pthread_exit(nullptr);
            return;
        }

        if (!tryContextSwitch(queue, uc)) {
            return; // No task to switch to
        }

        signals += 1;
    });

    task::SchedulerEntry task0;
    task::SchedulerEntry task1;

    status = enqueueCounterTask(&state0, TestThread0, &task0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = enqueueCounterTask(&state1, TestThread1, &task1);
    ASSERT_EQ(status, OsStatusSuccess);

    runQueue(thread, [&](auto elapsed) {
        if (state0.counter.load() == 0 || state1.counter.load() == 0) {
            return false;
        }

        if (elapsed > std::chrono::milliseconds(100)) {
            task0.terminate();
            task1.terminate();
        }

        // Once the task is terminated, it should not be rescheduled.
        if (task0.isClosed()) {
            if (savedCounter0.load() == 0) {
                savedCounter0.store(state0.counter.load());
            } else {
                EXPECT_EQ(savedCounter0.load(), state0.counter.load());
            }
        }

        return task0.isClosed() && task1.isClosed() && elapsed > std::chrono::milliseconds(250);
    });

    done.store(true);

    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    EXPECT_NE(signals.load(), 0);
    EXPECT_NE(state0.counter.load(), 0);
    EXPECT_NE(state1.counter.load(), 0);
    EXPECT_NE(savedCounter0.load(), 0);
    EXPECT_EQ(state0.counter.load(), savedCounter0.load());
    EXPECT_TRUE(task0.isClosed());
    EXPECT_TRUE(task1.isClosed());
}

TEST_F(SchedulerTest, ManyThreads) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;

    static constexpr size_t kMaxThreads = 128;
    std::vector<std::unique_ptr<ThreadTestState>> states;
    states.reserve(kMaxThreads);
    for (size_t i = 0; i < kMaxThreads; ++i) {
        auto& state = states.emplace_back(std::make_unique<ThreadTestState>());
        initTestState(state.get());
    }

    std::vector<task::SchedulerEntry> entries{kMaxThreads};

    std::atomic<bool> done = false;

    auto [thread, handle] = ktest::CreateReentrantThreadPair([&] {
        while (!done.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, ktest::kReentrantSignal);
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    }, [&]([[maybe_unused]] siginfo_t *siginfo, ucontext_t *uc) {
        if (done.load()) {
            pthread_exit(nullptr);
            return;
        }

        if (!tryContextSwitch(queue, uc)) {
            return; // No task to switch to
        }

        signals += 1;
    });

    std::atomic<size_t> index = 0;
    for (size_t i = 0; i < 3; i++) {
        size_t next = index.fetch_add(1);
        status = enqueueCounterTask(states[next].get(), TestThread0, &entries[next]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    size_t maxIndex = 0;

    runQueue(thread, [&](auto elapsed) {
        size_t next = index.fetch_add(1);
        if (next < kMaxThreads) {
            status = enqueueCounterTask(states[next].get(), TestThread1, &entries[next]);
            if (status == OsStatusOutOfMemory) {
                index = kMaxThreads + 1; // Stop adding more threads
            } else {
                maxIndex = std::max(maxIndex, next);
            }
        }

        for (size_t i = 0; i <= maxIndex; ++i) {
            if (states[i]->counter.load() == 0) {
                return false; // Not all threads have run yet
            }
        }

        return elapsed > std::chrono::milliseconds(250);
    });

    done.store(true);

    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    EXPECT_NE(signals.load(), 0);
    for (size_t i = 0; i <= maxIndex; ++i) {
        EXPECT_NE(states[i]->counter.load(), 0) << "Thread " << i << " did not run";
    }
}

TEST_F(SchedulerTest, ScheduleFrequency) {
    OsStatus status = OsStatusSuccess;
    std::atomic<size_t> signals = 0;

    static constexpr size_t kMaxThreads = 32;
    std::vector<std::unique_ptr<ThreadTestState>> states;
    states.reserve(kMaxThreads);
    for (size_t i = 0; i < kMaxThreads; ++i) {
        auto& state = states.emplace_back(std::make_unique<ThreadTestState>());
        initTestState(state.get());
    }

    std::vector<task::SchedulerEntry> entries{kMaxThreads};

    std::atomic<bool> done = false;
    std::atomic<bool> error = false;
    std::atomic<bool> missingTask = false;
    std::map<task::SchedulerEntry*, size_t> threadScheduleCount;

    for (size_t i = 0; i < 32; i++) {
        status = enqueueCounterTask(states[i].get(), TestThread0, &entries[i]);
        threadScheduleCount[&entries[i]] = 0;
        ASSERT_EQ(status, OsStatusSuccess);
    }

    auto [thread, handle] = ktest::CreateReentrantThreadPair([&] {
        while (!done.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, ktest::kReentrantSignal);
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    }, [&]([[maybe_unused]] siginfo_t *siginfo, ucontext_t *uc) {
        if (done.load()) {
            pthread_exit(nullptr);
            return;
        }

        if (!tryContextSwitch(queue, uc)) {
            error = true;
            return; // No task to switch to
        }

        if (!threadScheduleCount.contains(queue.getCurrentTask())) {
            missingTask = true;
        }
        threadScheduleCount[queue.getCurrentTask()] += 1;

        signals += 1;
    });

    size_t maxIndex = 0;

    runQueue(thread, [&](auto elapsed) {
        for (size_t i = 0; i <= maxIndex; ++i) {
            if (states[i]->counter.load() == 0) {
                return false; // Not all threads have run yet
            }
        }

        return elapsed > std::chrono::milliseconds(250);
    });

    done.store(true);

    for (size_t i = 0; i < 10; ++i) {
        ktest::AlertReentrantThread(thread);
    }
    pthread_join(thread, nullptr);
    delete handle;

    ASSERT_FALSE(error) << "An error occurred during the test";
    ASSERT_FALSE(missingTask) << "A task was not found in the thread schedule count";

    EXPECT_NE(signals.load(), 0);
    for (size_t i = 0; i <= maxIndex; ++i) {
        EXPECT_NE(states[i]->counter.load(), 0) << "Thread " << i << " did not run";
    }

    size_t sum = 0;

    for (const auto& [entry, count] : threadScheduleCount) {
        sum += count;
    }

    size_t avg = sum / threadScheduleCount.size();
    size_t lowerBound = avg * 9 / 10;
    size_t upperBound = avg * 11 / 10;

    for (const auto& [entry, count] : threadScheduleCount) {
        EXPECT_GE(count, lowerBound) << "Thread " << entry << " was scheduled too often";
        EXPECT_LE(count, upperBound) << "Thread " << entry << " was scheduled too infrequently";
    }
}
