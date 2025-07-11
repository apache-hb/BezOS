#include <gtest/gtest.h>
#include <latch>
#include <thread>

#include "task/scheduler.hpp"
#include "pthread.hpp"
#include "reentrant.hpp"

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
}

[[gnu::target("general-regs-only")]]
static void TestThread0(void *arg) {
    ThreadTestState *state = static_cast<ThreadTestState *>(arg);
    // printf("TestThread0: %p\n", (void*)entry);
    while (true) {
        state->counter += 1;

        // ASSERT_EQ(queue->getCurrentTask(), entry);
    }

    while (true) {

    }
}

class SchedulerTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
    }

    void SetUp() override {
        for (size_t i = 0; i < kQueueMaxTasks; i++) {
            initTestState(&states[i]);
            states[i].entry = &entries[i];
        }

        for (size_t i = 0; i < kQueueCount; i++) {
            ASSERT_EQ(task::SchedulerQueue::create(kQueueCapacity, &queues[i]), OsStatusSuccess);
            scheduler.addQueue(km::CpuCoreId(i), &queues[i]);
        }
    }

    OsStatus enqueueCounterTask(ThreadTestState *state, void (*task)(void *), task::SchedulerEntry *entry) {
        state->entry = entry;
        x64::page *stack = state->stackStorage.get();
        x64::page *xsave = state->xsaveStorage.get();

        OsStatus status = scheduler.enqueue({
            .registers = {
                .rdi = reinterpret_cast<uintptr_t>(state),
                .rbp = reinterpret_cast<uintptr_t>(stack + 4) - 0x8,
                .rsp = reinterpret_cast<uintptr_t>(stack + 4) - 0x8,
                .rip = reinterpret_cast<uintptr_t>(task),
            },
            .xsave = (x64::XSave*)(xsave),
        }, km::StackMappingAllocation{}, km::StackMappingAllocation{}, entry);
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

    static constexpr size_t kQueueCount = 4;
    static constexpr size_t kQueueCapacity = 8;
    static constexpr size_t kQueueTaskCount = (kQueueCapacity / 2);
    static constexpr size_t kQueueMaxTasks = kQueueCount * kQueueTaskCount;

    task::Scheduler scheduler;
    std::unique_ptr<task::SchedulerQueue[]> queues{ new task::SchedulerQueue[kQueueCount] };
    std::unique_ptr<task::SchedulerEntry[]> entries{ new task::SchedulerEntry[kQueueMaxTasks] };
    std::unique_ptr<ThreadTestState[]> states{ new ThreadTestState[kQueueMaxTasks] };
    std::vector<ktest::PThread> threads{ kQueueCount };
};

[[gnu::target("general-regs-only"), gnu::naked]]
static void IdleThread() {
    asm("1:\n jmp 1b");
}

static bool tryContextSwitch(task::Scheduler& scheduler, km::CpuCoreId id, ucontext_t *uc) {
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

    if (scheduler.reschedule(id, &state) == task::ScheduleResult::eIdle) {
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

static thread_local km::CpuCoreId tlsCpuCoreId;
static task::Scheduler *gScheduler = nullptr;
static std::atomic<bool> done{false};
static std::atomic<size_t> signals{0};

TEST_F(SchedulerTest, Schedule) {
    std::latch ready(kQueueCount);
    gScheduler = &scheduler;

    struct sigaction sigusr1 {
        .sa_sigaction = [](int, [[maybe_unused]] siginfo_t *siginfo, [[maybe_unused]] void *ctx) {
            ucontext_t *uc = reinterpret_cast<ucontext_t *>(ctx);
            if (done.load()) {
                pthread_exit(nullptr);
                return;
            }

            if (!tryContextSwitch(*gScheduler, tlsCpuCoreId, uc)) {
                return;
            }

            signals += 1;
        },
        .sa_flags = SA_SIGINFO,
    };

    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    }

    sigaction(SIGUSR1, &sigusr1, nullptr);

    threads.resize(kQueueCount);

    for (size_t i = 0; i < kQueueCount; ++i) {
        threads[i] = ktest::PThread(10, [i, &ready]() {
            tlsCpuCoreId = km::CpuCoreId(i);

            sigset_t set;
            sigemptyset(&set);
            sigaddset(&set, SIGUSR1);
            sigprocmask(SIG_UNBLOCK, &set, nullptr);

            ready.arrive_and_wait();

            while (!done.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        for (size_t j = 0; j < kQueueTaskCount; ++j) {
            size_t index = i * kQueueTaskCount + j;
            auto& state = states[index];
            task::SchedulerEntry& entry = entries[index];

            initTestState(&state);
            state.entry = &entry;
            state.queue = &queues[i];

            OsStatus status = enqueueCounterTask(&state, TestThread0, &entry);
            ASSERT_EQ(status, OsStatusSuccess);
        }
    }

    ready.wait();

    std::vector<ktest::PThread> carriers;
    for (ktest::PThread& thread : threads) {
        carriers.emplace_back(ktest::PThread(20, [this, handle = thread.getHandle()]() {
            runQueue(handle, [](auto elapsed) -> bool {
                return elapsed > std::chrono::milliseconds(100);
            });
        }));
    }

    carriers.clear(); // Wait for all threads to finish
    done.store(true);
    for (ktest::PThread& thread : threads) {
        for (size_t i = 0; i < 10; ++i) {
            ktest::AlertReentrantThread(thread.getHandle());
        }
    }

    for (size_t i = 0; i < kQueueMaxTasks; ++i) {
        EXPECT_NE(states[i].counter.load(), 0) << "Thread " << i << " did not run";
        printf("Thread %zu ran %zu times\n", i, states[i].counter.load());
    }
    EXPECT_NE(signals.load(), 0);
}
