#include <gtest/gtest.h>
#include <latch>
#include <random>
#include <thread>

#include "common/compiler/compiler.hpp"

#include "reentrant.hpp"

CLANG_DIAGNOSTIC_PUSH();
// strings move assignment isnt nonblocking so clang rightly complains
// this is fine for tests, but in real code only nonblocking move assignable types
// should be allowed in the ring buffer
CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

#include "std/ringbuffer.hpp"

CLANG_DIAGNOSTIC_POP();

TEST(RingBufferConstructTest, Construct) {
    sm::AtomicRingQueue<int> queue;
    OsStatus status = sm::AtomicRingQueue<int>::create(1024, &queue);
    ASSERT_EQ(OsStatusSuccess, status);
    ASSERT_EQ(queue.capacity(), 1024);
    ASSERT_EQ(queue.count(), 0);
}

TEST(RingBufferConstructTest, ConstructString) {
    sm::AtomicRingQueue<std::string> queue;
    OsStatus status = sm::AtomicRingQueue<std::string>::create(1024, &queue);
    ASSERT_EQ(OsStatusSuccess, status);
    ASSERT_EQ(queue.capacity(), 1024);
    ASSERT_EQ(queue.count(), 0);
}

class RingBufferTest : public testing::Test {
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

TEST_F(RingBufferTest, Push) {
    std::string value = "Hello, World!";
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);
}

TEST_F(RingBufferTest, Pop) {
    std::string data = "Hello, World!";
    std::string value = auto{data};
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);

    std::string poppedValue;
    ASSERT_TRUE(queue.tryPop(poppedValue));
    ASSERT_EQ(poppedValue, data);
    ASSERT_EQ(queue.count(), 0);
}

TEST_F(RingBufferTest, PushFull) {
    for (size_t i = 0; i < kCapacity; ++i) {
        std::string value = "Hello, World!";
        ASSERT_TRUE(queue.tryPush(value));
    }
    ASSERT_EQ(queue.count(), kCapacity);

    std::string value = "This should not be pushed";
    ASSERT_FALSE(queue.tryPush(value));
}

TEST_F(RingBufferTest, PopEmpty) {
    std::string value;
    ASSERT_FALSE(queue.tryPop(value));
    ASSERT_EQ(queue.count(), 0);
}

TEST_F(RingBufferTest, ThreadSafe) {
    constexpr size_t kProducerCount = 8;
    std::vector<std::jthread> producers;
    std::latch latch(kProducerCount + 1);

    std::atomic<size_t> producedCount = 0;
    std::atomic<size_t> consumedCount = 0;
    std::atomic<size_t> droppedCount = 0;

    for (size_t i = 0; i < kProducerCount; ++i) {
        producers.emplace_back([&] {
            latch.arrive_and_wait();

            for (size_t j = 0; j < 1000; ++j) {
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

struct State {
    std::atomic<size_t> signals{0};
    std::atomic<size_t> inThread{0};
    std::atomic<size_t> inMainThread{0};
    std::atomic<bool> done{false};
    sm::AtomicRingQueue<std::string> *queue;

    pthread_t CreateTestThread() {
        return ktest::CreateReentrantThread([&] {
            while (!done.load()) {
                std::string value = "Hello, World!";
                if (queue->tryPop(value)) {
                    queue->tryPush(value);
                }

                inThread += 1;
            }
        }, [&]([[maybe_unused]] siginfo_t *siginfo, [[maybe_unused]] ucontext_t *uc) {
            std::string value = "From signal handler";
            if (queue->tryPop(value)) {
                queue->tryPush(value);
            }

            signals += 1;
        });
    }
};

TEST_F(RingBufferTest, Reentrant) {
    for (int i = 0; i < 100; i++) {
        std::string value = "Hello, World! " + std::to_string(i);
        if (queue.tryPush(value)) {
            break;
        }
    }

    State state {.queue = &queue};

    pthread_t thread = state.CreateTestThread();

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::milliseconds(50);
    while (now < end) {
        std::string value = "Hello, World!";
        queue.tryPush(value);
        ktest::AlertReentrantThread(thread);
        now = std::chrono::high_resolution_clock::now();

        queue.tryPop(value);

        state.inMainThread += 1;
    }

    state.done.store(true);
    pthread_join(thread, nullptr);

    ASSERT_NE(state.signals.load(), 0);
    ASSERT_NE(state.inThread.load(), 0);
    ASSERT_NE(state.inMainThread.load(), 0);
}

TEST_F(RingBufferTest, MultiThreadReentrant) {
    constexpr size_t kProducerCount = 8;

    for (int i = 0; i < 100; i++) {
        std::string value = "Hello, World! " + std::to_string(i);
        if (queue.tryPush(value)) {
            break;
        }
    }

    State state {.queue = &queue};
    std::vector<pthread_t> threads;

    for (size_t i = 0; i < kProducerCount; i++) {
        pthread_t thread = state.CreateTestThread();
        threads.push_back(thread);
    }

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::milliseconds(50);
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<size_t> dist(0, kProducerCount - 1);
    while (now < end) {
        std::string value = "Hello, World!";
        queue.tryPush(value);
        ktest::AlertReentrantThread(threads[dist(mt)]);
        now = std::chrono::high_resolution_clock::now();

        queue.tryPop(value);

        state.inMainThread += 1;
    }

    state.done.store(true);
    for (pthread_t thread : threads) {
        pthread_join(thread, nullptr);
    }

    ASSERT_NE(state.signals.load(), 0);
    ASSERT_NE(state.inThread.load(), 0);
    ASSERT_NE(state.inMainThread.load(), 0);
}
