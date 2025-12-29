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

class RingBufferDetailsTest : public testing::TestWithParam<size_t> {
public:
    std::unique_ptr<std::atomic<uint64_t>[]> bitset;

    void SetUp() override {
        size_t size = getSize();
        bitset = std::make_unique<std::atomic<uint64_t>[]>(size);
        std::uninitialized_fill_n(bitset.get(), size, 0);
    }

    size_t getSize() const {
        return sm::detail::requiredBitsetSize(getCapacity());
    }

    size_t getCapacity() const {
        return GetParam();
    }
};

TEST_P(RingBufferDetailsTest, AtomicScanAndSet) {
    std::vector<bool> allocated;
    allocated.resize(getCapacity(), false);

    for (size_t i = 0; i < getCapacity(); i++) {
        size_t index = sm::detail::atomicScanAndSet(bitset.get(), getCapacity());

        // we don't care *which* index we get, just that we get a valid one
        ASSERT_NE(index, (std::numeric_limits<size_t>::max)()) << "Failed to allocate at iteration " << i;

        ASSERT_FALSE(allocated[index]) << "Allocated index " << index << " twice";
        allocated[index] = true;
    }

    // now the bitset should be full
    size_t index = sm::detail::atomicScanAndSet(bitset.get(), getCapacity());
    ASSERT_EQ(index, (std::numeric_limits<size_t>::max)()) << "Allocated index when full";
}

INSTANTIATE_TEST_SUITE_P(
    RingBufferDetailsTests,
    RingBufferDetailsTest,
    testing::Values(1, 2, 4, 8, 16, 32, 64, 65, 128, 256, 512, 1024));

class RingBufferSizedTest : public testing::TestWithParam<size_t> {
public:
    sm::AtomicRingQueue<std::string> queue;

    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
        setbuf(stderr, nullptr);
    }

    void SetUp() override {
        size_t capacity = GetParam();
        OsStatus status = sm::AtomicRingQueue<std::string>::create(capacity, &queue);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(queue.capacity(), capacity);
        ASSERT_EQ(queue.count(), 0);
    }
};

TEST_P(RingBufferSizedTest, Push) {
    std::string value = "Hello, World!";
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);
}

TEST_P(RingBufferSizedTest, Pop) {
    std::string data = "Hello, World!";
    std::string value = auto{data};
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);

    std::string poppedValue;
    ASSERT_TRUE(queue.tryPop(poppedValue));
    ASSERT_EQ(poppedValue, data);
    ASSERT_EQ(queue.count(), 0);
}

TEST_P(RingBufferSizedTest, PushFull) {
    for (size_t i = 0; i < queue.capacity(); ++i) {
        std::string value = "Hello, World!";
        ASSERT_TRUE(queue.tryPush(value)) << "Failed to push at index " << i;
    }
    ASSERT_EQ(queue.count(), queue.capacity());

    std::string value = "This should not be pushed";
    ASSERT_FALSE(queue.tryPush(value));
}

TEST_P(RingBufferSizedTest, PushInOrder) {
    for (size_t i = 0; i < queue.capacity(); i++) {
        std::string value = "Hello, World! " + std::to_string(i);
        ASSERT_TRUE(queue.tryPush(value)) << "Failed to push at index " << i;
    }
    ASSERT_EQ(queue.count(), queue.capacity());

    for (size_t i = 0; i < queue.capacity(); i++) {
        std::string expected = "Hello, World! " + std::to_string(i);
        std::string value;
        ASSERT_TRUE(queue.tryPop(value));
        ASSERT_EQ(expected, value) << "Unexpected value at index " << i;
    }

    ASSERT_EQ(queue.count(), 0);

    std::string value;
    ASSERT_FALSE(queue.tryPop(value));
}

TEST_P(RingBufferSizedTest, PopEmpty) {
    std::string value;
    ASSERT_FALSE(queue.tryPop(value));
    ASSERT_EQ(queue.count(), 0);
}

INSTANTIATE_TEST_SUITE_P(
    RingBufferTests,
    RingBufferSizedTest,
    testing::Values(1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024));

template<typename T, size_t N>
struct MessageValues {
    static constexpr size_t kMaxValues = N;

    std::array<T, kMaxValues> consumed{};
    std::atomic<size_t> consumedIndex{0};

    std::array<T, kMaxValues> produced{};
    std::atomic<size_t> producedIndex{0};

    void recordConsumed(T value) {
        size_t index = consumedIndex.fetch_add(1);
        if (index < kMaxValues) {
            consumed[index] = value;
        }
    }

    void recordProduced(T value) {
        size_t index = producedIndex.fetch_add(1);
        if (index < kMaxValues) {
            produced[index] = value;
        }
    }

    void assertEqual() {
        ASSERT_EQ(producedIndex.load(), consumedIndex.load());

        std::sort(consumed.begin(), consumed.begin() + consumedIndex.load());
        std::sort(produced.begin(), produced.begin() + producedIndex.load());

        for (size_t i = 0; i < consumedIndex.load(); i++) {
            ASSERT_EQ(consumed[i], produced[i])
                << "Mismatch at index " << i
                << " (" << producedIndex.load() << "/" << consumedIndex.load() << ")";
        }
    }
};

class RingBufferThreadTest : public testing::Test {
public:
    using Element = size_t;
    using TestQueue = sm::AtomicRingQueue<Element>;

    TestQueue queue;
    static constexpr size_t kCapacity = 1024;
    static constexpr size_t kProducerCount = 8;

    MessageValues<Element, 0x1000 * 4> messageValues;
    std::atomic<Element> nextValue{1};

    void SetUp() override {
        OsStatus status = TestQueue::create(1024, &queue);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(queue.capacity(), 1024);
        ASSERT_EQ(queue.count(), 0);
    }

    void recordConsumed(Element value) {
        messageValues.recordConsumed(value);
    }

    void recordProduced(Element value) {
        messageValues.recordProduced(value);
    }
};

TEST_F(RingBufferThreadTest, ThreadSafe) {
    std::vector<std::jthread> producers;
    std::latch latch{kProducerCount + 1};

    std::atomic<size_t> producedCount = 0;
    std::atomic<size_t> consumedCount = 0;
    std::atomic<size_t> droppedCount = 0;

    for (size_t i = 0; i < kProducerCount; ++i) {
        producers.emplace_back([&] {
            latch.arrive_and_wait();

            for (size_t j = 0; j < 1000; ++j) {
                Element value = nextValue.fetch_add(1);
                if (queue.tryPush(value)) {
                    producedCount += 1;
                    recordProduced(value);
                } else {
                    droppedCount += 1;
                }
            }
        });
    }

    {
        std::jthread consumer([&](std::stop_token stop) {
            latch.arrive_and_wait();

            Element value{std::numeric_limits<Element>::max()};
            while (!stop.stop_requested()) {
                if (queue.tryPop(value)) {
                    consumedCount += 1;
                    recordConsumed(value);
                }
            }
        });

        producers.clear();
    }

    Element value{std::numeric_limits<Element>::max()};
    while (queue.tryPop(value)) {
        consumedCount += 1;
        recordConsumed(value);
    }

    {
        auto it = std::find(messageValues.produced.begin(), messageValues.produced.end(), std::numeric_limits<Element>::max());
        ASSERT_EQ(it, messageValues.produced.end()) << "Produced sentinel value found in produced messages at " << std::distance(messageValues.produced.begin(), it);
    }

    {
        auto it = std::find(messageValues.consumed.begin(), messageValues.consumed.end(), std::numeric_limits<Element>::max());
        ASSERT_EQ(it, messageValues.consumed.end()) << "Consumed sentinel value found in consumed messages at " << std::distance(messageValues.consumed.begin(), it);
    }

    messageValues.assertEqual();

    ASSERT_NE(producedCount.load(), 0);
    ASSERT_EQ(consumedCount.load(), producedCount.load());
}

struct State {
    std::atomic<size_t> signals{0};
    std::atomic<size_t> inThread{0};
    std::atomic<size_t> inMainThread{0};

    std::atomic<size_t> produceCount{0};
    std::atomic<size_t> consumeCount{0};

    std::atomic<bool> done{false};
    sm::AtomicRingQueue<std::string> *queue;

    pthread_t CreateTestThread() {
        return ktest::CreateReentrantThread([&] {
            while (!done.load()) {
                std::string value = "Hello, World!";
                if (queue->tryPush(value)) {
                    produceCount += 1;
                }

                inThread += 1;
            }
        }, [&]([[maybe_unused]] siginfo_t *siginfo, [[maybe_unused]] ucontext_t *uc) {
            std::string value = "From signal handler";
            if (queue->tryPush(value)) {
                produceCount += 1;
            }

            signals += 1;
        });
    }
};

class RingBufferTest : public testing::Test {
public:
    static constexpr size_t kCapacity = 1024;
    sm::AtomicRingQueue<std::string> queue;

    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
        setbuf(stderr, nullptr);
    }

    void SetUp() override {
        OsStatus status = sm::AtomicRingQueue<std::string>::create(kCapacity, &queue);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(queue.capacity(), kCapacity);
        ASSERT_EQ(queue.count(), 0);
    }
};

TEST_F(RingBufferTest, Reentrant) {
    State state {.queue = &queue};

    for (int i = 0; i < 100; i++) {
        std::string value = "Hello, World! " + std::to_string(i);
        if (!queue.tryPush(value)) {
            break;
        }
        state.produceCount += 1;
    }

    pthread_t thread = state.CreateTestThread();

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::milliseconds(50);
    while (now < end) {
        std::string value = "Hello, World!";
        if (queue.tryPush(value)) {
            state.produceCount += 1;
        }

        ktest::AlertReentrantThread(thread);
        now = std::chrono::high_resolution_clock::now();

        if (queue.tryPop(value)) {
            state.consumeCount += 1;
        }

        state.inMainThread += 1;
    }

    state.done.store(true);
    pthread_join(thread, nullptr);

    std::string value;
    while (queue.tryPop(value)) {
        state.consumeCount += 1;
    }

    ASSERT_NE(state.signals.load(), 0);
    ASSERT_NE(state.inThread.load(), 0);
    ASSERT_NE(state.inMainThread.load(), 0);

    ASSERT_EQ(state.produceCount.load(), state.consumeCount.load());
    ASSERT_NE(state.produceCount.load(), 0);
    ASSERT_NE(state.consumeCount.load(), 0);
}

TEST_F(RingBufferTest, MultiThreadReentrant) {
    constexpr size_t kProducerCount = 8;

    State state {.queue = &queue};

    for (int i = 0; i < 100; i++) {
        std::string value = "Hello, World! " + std::to_string(i);
        if (!queue.tryPush(value)) {
            break;
        }
        state.produceCount += 1;
    }

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
        if (queue.tryPush(value)) {
            state.produceCount += 1;
        }

        ktest::AlertReentrantThread(threads[dist(mt)]);
        now = std::chrono::high_resolution_clock::now();

        if (queue.tryPop(value)) {
            state.consumeCount += 1;
        }

        state.inMainThread += 1;
    }

    state.done.store(true);
    for (pthread_t thread : threads) {
        pthread_join(thread, nullptr);
    }

    std::string value;
    while (queue.tryPop(value)) {
        state.consumeCount += 1;
    }

    ASSERT_NE(state.signals.load(), 0);
    ASSERT_NE(state.inThread.load(), 0);
    ASSERT_NE(state.inMainThread.load(), 0);
    ASSERT_EQ(state.produceCount.load(), state.consumeCount.load());
    ASSERT_NE(state.produceCount.load(), 0);
    ASSERT_NE(state.consumeCount.load(), 0);
}

TEST(RingBufferOrderTest, Order) {
    sm::AtomicRingQueue<size_t> queue;
    OsStatus status = sm::AtomicRingQueue<size_t>::create(64, &queue);
    ASSERT_EQ(OsStatusSuccess, status);

    for (size_t i = 0; i < 64; i++) {
        size_t value = i * 10;
        ASSERT_TRUE(queue.tryPush(value)) << "Failed to push at index " << i;
    }

    ASSERT_EQ(queue.count(), 64);
    for (size_t i = 0; i < 64; i++) {
        size_t value;
        ASSERT_TRUE(queue.tryPop(value));
        ASSERT_EQ(value, i * 10) << "Value at index " << i << " is incorrect";
    }
}

class RingBufferReentrancyTest : public testing::Test {
public:
    using Element = size_t;
    using TestQueue = sm::AtomicRingQueue<Element>;

    TestQueue queue;
    static constexpr size_t kCapacity = 1024;

    void SetUp() override {
        OsStatus status = TestQueue::create(1024, &queue);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(queue.capacity(), 1024);
        ASSERT_EQ(queue.count(), 0);
    }

    std::atomic<size_t> signals{0};
    std::atomic<size_t> inThread{0};
    std::atomic<size_t> inMainThread{0};

    std::atomic<size_t> produceCount{0};
    std::atomic<size_t> consumeCount{0};

    std::atomic<bool> done{false};

    static constexpr size_t kMaxValues = 0x1000 * 4;

    std::array<Element, kMaxValues> consumed{};
    std::atomic<size_t> consumedIndex{0};

    std::array<Element, kMaxValues> produced{};
    std::atomic<size_t> producedIndex{0};

    std::atomic<Element> nextValue{1};

    size_t getConsumedIndex() const {
        return std::min(consumedIndex.load(), kMaxValues);
    }

    size_t getProducedIndex() const {
        return std::min(producedIndex.load(), kMaxValues);
    }

    void recordConsumed(Element value) {
        size_t index = consumedIndex.fetch_add(1);
        if (index < kMaxValues) {
            consumed[index] = value;
        }
        consumeCount += 1;
    }

    void recordProduced(Element value) {
        size_t index = producedIndex.fetch_add(1);
        if (index < kMaxValues) {
            produced[index] = value;
        }
        produceCount += 1;
    }

    pthread_t CreateTestThread() {
        return ktest::CreateReentrantThread([&] {
            while (!done.load()) {
                Element value = nextValue.fetch_add(1);
                if (queue.tryPush(value)) {
                    recordProduced(value);
                }

                inThread += 1;
            }
        }, [&]([[maybe_unused]] siginfo_t *siginfo, [[maybe_unused]] ucontext_t *uc) {
            Element value = nextValue.fetch_add(1);
            if (queue.tryPush(value)) {
                recordProduced(value);
            }

            signals += 1;
        });
    }
};

TEST_F(RingBufferReentrancyTest, Reentrant) {
    for (int i = 0; i < 100; i++) {
        Element value = nextValue.fetch_add(1);
        if (queue.tryPush(value)) {
            recordProduced(value);
        } else {
            break;
        }
    }

    pthread_t thread = CreateTestThread();

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::milliseconds(50);
    while (now < end) {
        Element value = nextValue.fetch_add(1);
        if (queue.tryPush(value)) {
            recordProduced(value);
        }

        ktest::AlertReentrantThread(thread);
        now = std::chrono::high_resolution_clock::now();

        value = std::numeric_limits<Element>::max();
        if (queue.tryPop(value)) {
            recordConsumed(value);
        }

        inMainThread += 1;
    }

    done.store(true);
    pthread_join(thread, nullptr);

    Element value{std::numeric_limits<Element>::max()};
    while (queue.tryPop(value)) {
        recordConsumed(value);
        value = std::numeric_limits<Element>::max();
    }

    RecordProperty("Produced", std::format("{}", produceCount.load()));
    RecordProperty("Consumed", std::format("{}", consumeCount.load()));

    ASSERT_EQ(produceCount.load(), producedIndex.load());
    ASSERT_EQ(consumeCount.load(), consumedIndex.load());
    ASSERT_NE(consumedIndex.load(), 0);
    ASSERT_NE(producedIndex.load(), 0);

    std::sort(consumed.begin(), consumed.begin() + getConsumedIndex());
    std::sort(produced.begin(), produced.begin() + getProducedIndex());

    for (size_t i = 0; i < getConsumedIndex(); i++) {
        EXPECT_NE(consumed[i], std::numeric_limits<Element>::max()) << "Consumed sentinel value found at index " << i;
        EXPECT_NE(produced[i], std::numeric_limits<Element>::max()) << "Produced sentinel value found at index " << i;
        ASSERT_EQ(consumed[i], produced[i])
            << "Mismatch at index " << i
            << " (" << getProducedIndex() << "/" << getConsumedIndex() << ")";
    }

    ASSERT_NE(signals.load(), 0);
    ASSERT_NE(inThread.load(), 0);
    ASSERT_NE(inMainThread.load(), 0);

    ASSERT_EQ(produceCount.load(), consumeCount.load());
    ASSERT_NE(produceCount.load(), 0);
    ASSERT_NE(consumeCount.load(), 0);
}
