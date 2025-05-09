#include <gtest/gtest.h>
#include <latch>
#include <thread>

#include "common/compiler/compiler.hpp"

CLANG_DIAGNOSTIC_PUSH();
// strings move assignment isnt nonblocking so clang rightly complains
// this is fine for tests, but in real code only nonblocking move assignable types
// should be allowed in the ring buffer
CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

#include "std/ringbuffer.hpp"

CLANG_DIAGNOSTIC_POP();

TEST(RingBufferTest, Construct) {
    sm::AtomicRingQueue<int> queue;
    OsStatus status = sm::AtomicRingQueue<int>::create(1024, &queue);
    ASSERT_EQ(OsStatusSuccess, status);
    ASSERT_EQ(queue.capacity(), 1024);
    ASSERT_EQ(queue.count(), 0);
}

TEST(RingBufferTest, ConstructString) {
    sm::AtomicRingQueue<std::string> queue;
    OsStatus status = sm::AtomicRingQueue<std::string>::create(1024, &queue);
    ASSERT_EQ(OsStatusSuccess, status);
    ASSERT_EQ(queue.capacity(), 1024);
    ASSERT_EQ(queue.count(), 0);
}

class RingBufferStringTest : public testing::Test {
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

TEST_F(RingBufferStringTest, Push) {
    std::string value = "Hello, World!";
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);
}

TEST_F(RingBufferStringTest, Pop) {
    std::string data = "Hello, World!";
    std::string value = auto{data};
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);

    std::string poppedValue;
    ASSERT_TRUE(queue.tryPop(poppedValue));
    ASSERT_EQ(poppedValue, data);
    ASSERT_EQ(queue.count(), 0);
}

TEST_F(RingBufferStringTest, PushFull) {
    for (size_t i = 0; i < kCapacity; ++i) {
        std::string value = "Hello, World!";
        ASSERT_TRUE(queue.tryPush(value));
    }
    ASSERT_EQ(queue.count(), kCapacity);

    std::string value = "This should not be pushed";
    ASSERT_FALSE(queue.tryPush(value));
}

TEST_F(RingBufferStringTest, PopEmpty) {
    std::string value;
    ASSERT_FALSE(queue.tryPop(value));
    ASSERT_EQ(queue.count(), 0);
}

TEST_F(RingBufferStringTest, ThreadSafe) {
    size_t producerCount = 8;
    std::vector<std::jthread> producers;
    std::latch latch(producerCount + 1);

    std::atomic<size_t> producedCount = 0;
    std::atomic<size_t> consumedCount = 0;
    std::atomic<size_t> droppedCount = 0;

    for (size_t i = 0; i < producerCount; ++i) {
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

    ASSERT_NE(producedCount.load(), 0);
    ASSERT_EQ(consumedCount.load(), producedCount.load());
}
