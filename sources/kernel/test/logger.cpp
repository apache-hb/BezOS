#include <gtest/gtest.h>
#include <latch>
#include <thread>

#include "logger/logger.hpp"

class TestAppender final : public km::ILogAppender {
public:
    std::vector<km::detail::LogMessage> mMessages;

    void write(const km::LogMessageView& message) noexcept override {
        mMessages.push_back({
            .level = message.level,
            .location = message.location,
            .logger = message.logger,
            .message = message.message,
        });
    }
};

TEST(LoggerConstructTest, Construct) {
    km::LogQueue queue;
    OsStatus status = km::LogQueue::create(32, &queue);
    EXPECT_EQ(status, OsStatusSuccess);

    TestAppender appender;
    queue.addAppender(&appender);
    km::Logger logger("TestLogger", &queue);
    EXPECT_EQ(logger.getName(), "TestLogger");
    EXPECT_EQ(queue.flush(), 0);
    EXPECT_EQ(appender.mMessages.size(), 0);
}

class LoggerTest : public testing::Test {
public:
    static constexpr size_t kQueueSize = 32;
    void SetUp() override {
        OsStatus status = km::LogQueue::create(kQueueSize, &queue);
        EXPECT_EQ(status, OsStatusSuccess);
        queue.addAppender(&appender);
    }

    void AssertMessage(size_t index, km::LogLevel level, std::string_view message) {
        ASSERT_GT(appender.mMessages.size(), index);
        EXPECT_EQ(appender.mMessages[index].level, level);
        EXPECT_EQ(appender.mMessages[index].message, message);
        EXPECT_EQ(appender.mMessages[index].logger, &logger);
    }

    km::LogQueue queue;
    TestAppender appender;
    km::Logger logger{"TestLogger", &queue};
};

TEST_F(LoggerTest, LogMessage) {
    logger.dbgf("Test message");
    queue.flush();
    EXPECT_EQ(appender.mMessages.size(), 1);
    AssertMessage(0, km::LogLevel::eDebug, "Test message");
}

TEST_F(LoggerTest, FormatSeverityLevels) {
    logger.dbgf("Debug message ", 25);
    logger.infof("Info message", km::present(false));
    logger.warnf("Warning message ", 1234);
    logger.errorf("Error message ", km::Hex(0xDEADBEEF), " more text");
    logger.fatalf("Fatal message ", km::enabled(true));

    queue.flush();
    EXPECT_EQ(appender.mMessages.size(), 5);

    AssertMessage(0, km::LogLevel::eDebug, std::string_view(km::concat<km::kLogMessageSize>("Debug message ", 25)));
    AssertMessage(1, km::LogLevel::eInfo, std::string_view(km::concat<km::kLogMessageSize>("Info message", km::present(false))));
    AssertMessage(2, km::LogLevel::eWarning, std::string_view(km::concat<km::kLogMessageSize>("Warning message ", 1234)));
    AssertMessage(3, km::LogLevel::eError, std::string_view(km::concat<km::kLogMessageSize>("Error message ", km::Hex(0xDEADBEEF), " more text")));
    AssertMessage(4, km::LogLevel::eFatal, std::string_view(km::concat<km::kLogMessageSize>("Fatal message ", km::enabled(true))));
}

TEST_F(LoggerTest, SeverityLevels) {
    logger.info("Info message");
    logger.warn("Warning message");
    logger.error("Error message");
    logger.fatal("Fatal message");

    EXPECT_EQ(appender.mMessages.size(), 4);

    AssertMessage(0, km::LogLevel::eInfo, "Info message");
    AssertMessage(1, km::LogLevel::eWarning, "Warning message");
    AssertMessage(2, km::LogLevel::eError, "Error message");
    AssertMessage(3, km::LogLevel::eFatal, "Fatal message");
}

TEST_F(LoggerTest, LoggerSubmit) {
    logger.submit(km::LogLevel::eDebug, "Direct submit message", std::source_location::current());
    EXPECT_EQ(appender.mMessages.size(), 1);
    AssertMessage(0, km::LogLevel::eDebug, "Direct submit message");
}

TEST_F(LoggerTest, QueueSubmit) {
    km::detail::LogMessage message {
        .level = km::LogLevel::eDebug,
        .location = std::source_location::current(),
        .logger = &logger,
        .message = "Submit message",
    };
    queue.recordMessage(message);
    EXPECT_EQ(queue.flush(), 1);
    EXPECT_EQ(appender.mMessages.size(), 1);
    AssertMessage(0, km::LogLevel::eDebug, "Submit message");
}

TEST_F(LoggerTest, ThreadSafe) {
    constexpr size_t kThreadCount = 4;
    std::latch latch(kThreadCount + 1);

    std::vector<std::jthread> threads;
    for (size_t i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([this, &latch]() {
            latch.arrive_and_wait();
            for (size_t j = 0; j < (kQueueSize / kThreadCount); ++j) {
                logger.info("Thread message");
            }
        });
    }

    latch.arrive_and_wait();
    threads.clear();
    queue.flush();
    EXPECT_EQ(appender.mMessages.size(), kQueueSize);
    for (size_t i = 0; i < kQueueSize; ++i) {
        AssertMessage(i, km::LogLevel::eInfo, "Thread message");
    }
}

TEST_F(LoggerTest, QueueOverflow) {
    for (size_t i = 0; i < kQueueSize * 2; ++i) {
        queue.recordMessage({
            .level = km::LogLevel::eInfo,
            .location = std::source_location::current(),
            .logger = &logger,
            .message = "Overflow message",
        });
    }
    queue.flush();
    EXPECT_EQ(queue.getDroppedCount(), kQueueSize);
    EXPECT_EQ(queue.getCommittedCount(), kQueueSize);
    EXPECT_EQ(appender.mMessages.size(), kQueueSize);
    for (size_t i = 0; i < kQueueSize; ++i) {
        AssertMessage(i, km::LogLevel::eInfo, "Overflow message");
    }
}

TEST_F(LoggerTest, ThreadSafeSubmitImmediate) {
    constexpr size_t kThreadCount = 4;
    std::latch latch(kThreadCount + 1);
    std::atomic<size_t> messageCount = 0;

    std::vector<std::jthread> threads;
    for (size_t i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([&]() {
            latch.arrive_and_wait();
            for (size_t j = 0; j < (kQueueSize / kThreadCount); ++j) {
                OsStatus status = logger.printImmediate("Thread message");
                switch (status) {
                case OsStatusSuccess:
                    messageCount += 1;
                    break;
                case OsStatusDeviceBusy:
                    // This is expected if the queue is full
                    break;
                default:
                    FAIL() << "Unexpected status: " << static_cast<int>(status);
                    break;
                }
            }
        });
    }

    latch.arrive_and_wait();
    threads.clear();
    queue.flush();
    EXPECT_EQ(appender.mMessages.size(), messageCount);
    for (size_t i = 0; i < messageCount; ++i) {
        AssertMessage(i, km::LogLevel::ePrint, "Thread message");
    }
}
