#include <gtest/gtest.h>
#include <latch>
#include <thread>
#include <fstream>
#include <dlfcn.h>

#include "allocator/tlsf.hpp"
#include "common/compiler/compiler.hpp"
#include "util/memory.hpp"

CLANG_DIAGNOSTIC_PUSH();
// strings move assignment isnt nonblocking so clang rightly complains
// this is fine for tests, but in real code only nonblocking move assignable types
// should be allowed in the ring buffer
CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

#include "std/ringbuffer.hpp"

CLANG_DIAGNOSTIC_POP();

class RingBufferSoakTest : public testing::Test {
public:
    sm::AtomicRingQueue<std::string> queue;
    static constexpr size_t kCapacity = 1024;

    void SetUp() override {
        OsStatus status = sm::AtomicRingQueue<std::string>::create(1024, &queue);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(queue.capacity(), 1024);
        ASSERT_EQ(queue.count(), 0);
    }
};

TEST_F(RingBufferSoakTest, MultithreadSoak) {
    size_t producerCount = 8;
    std::vector<std::jthread> producers;
    std::latch latch(producerCount + 1);

    std::atomic<size_t> producedCount = 0;
    std::atomic<size_t> consumedCount = 0;
    std::atomic<size_t> droppedCount = 0;

    for (size_t i = 0; i < producerCount; ++i) {
        producers.emplace_back([&](std::stop_token stop) {
            latch.arrive_and_wait();

            while (!stop.stop_requested()) {
                std::string value = "Hello, World!";
                if (queue.tryPush(value)) {
                    producedCount += 1;
                } else {
                    droppedCount += 1;
                }
            }
        });
    }

    std::jthread consumer([&](std::stop_token stop) {
        latch.arrive_and_wait();

        std::string value;
        while (!stop.stop_requested()) {
            if (queue.tryPop(value)) {
                consumedCount += 1;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    producers.clear();
    consumer.request_stop();
    consumer.join();

    std::string value;
    while (queue.tryPop(value)) {
        consumedCount += 1;
    }

    ASSERT_NE(producedCount.load(), 0);
    ASSERT_EQ(consumedCount.load(), producedCount.load());
}

TEST_F(RingBufferSoakTest, ReentrantSoak) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // allocate seperate memory that will only be used in the signal handler
    static std::unique_ptr<std::byte[]> memory{new std::byte[sm::megabytes(1).bytes()]};
    static mem::TlsfAllocator pool{memory.get(), sm::megabytes(1).bytes()};
    using Allocator = mem::AllocatorPointer<std::pair<const uintptr_t, uintptr_t>>;
    static std::map<uintptr_t, uintptr_t, std::less<uintptr_t>, Allocator> ipSamples{Allocator(&pool)};

    struct State {
        std::atomic<size_t> signals{0};
        std::atomic<size_t> inThread{0};
        std::atomic<size_t> inMainThread{0};
        std::atomic<bool> done{false};
        sm::AtomicRingQueue<std::string> *queue;
    };

    for (int i = 0; i < 100; i++) {
        std::string value = "Hello, World! " + std::to_string(i);
        if (queue.tryPush(value)) {
            break;
        }
    }

    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    }

    static State state {.queue = &queue};
    pthread_t thread;
    pthread_create(&thread, &attr, [](void*) -> void* {
        struct sigaction sigusr1 {
            .sa_sigaction = [](int, siginfo_t *, void *ctx) {
                std::string value = "From signal handler";
                if (state.queue->tryPop(value)) {
                    state.queue->tryPush(value);
                }

                state.signals += 1;

                ucontext_t *uc = reinterpret_cast<ucontext_t *>(ctx);
                mcontext_t *mc = &uc->uc_mcontext;
                uintptr_t ip = mc->gregs[REG_RIP];
                ipSamples[ip] += 1;
            },
            .sa_flags = SA_SIGINFO,
        };
        sigaction(SIGUSR1, &sigusr1, nullptr);

        {
            sigset_t set;
            sigemptyset(&set);
            sigaddset(&set, SIGUSR1);
            sigprocmask(SIG_UNBLOCK, &set, nullptr);
        }

        while (!state.done.load()) {
            std::string value = "Hello, World!";
            if (state.queue->tryPop(value)) {
                state.queue->tryPush(value);
            }

            state.inThread += 1;
        }
        return nullptr;
    }, nullptr);

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::seconds(10);
    while (now < end) {
        std::string value = "Hello, World!";
        queue.tryPush(value);
        ASSERT_EQ(pthread_sigqueue(thread, SIGUSR1, {0}), 0);
        now = std::chrono::high_resolution_clock::now();

        queue.tryPop(value);

        state.inMainThread += 1;
    }

    state.done.store(true);
    pthread_join(thread, nullptr);
    pthread_attr_destroy(&attr);

    ASSERT_NE(state.signals.load(), 0);
    ASSERT_NE(state.inThread.load(), 0);
    ASSERT_NE(state.inMainThread.load(), 0);

    ASSERT_FALSE(ipSamples.empty());

    std::ofstream file("ringbuffer_soak_ip_samples.txt");

    for (const auto &[ip, count] : ipSamples) {
        Dl_info info;
        dladdr((void*)ip, &info);
        size_t baseAddress = (size_t)info.dli_fbase;
        size_t relativeAddress = ip - baseAddress;
        file << std::hex << relativeAddress << " " << std::dec << count << "\n";
    }
}
