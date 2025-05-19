#pragma once

#include "logger/appender.hpp"

#include "std/inlined_vector.hpp"
#include "std/ringbuffer.hpp"
#include "std/spinlock.hpp"

#include "util/format.hpp"

namespace km {
    class LogQueue {
        using MessageQueue = sm::AtomicRingQueue<detail::LogMessage>;
        using AppenderList = sm::InlinedVector<ILogAppender*, 4>;

        [[clang::no_destroy]]
        constinit static LogQueue sLogQueue;

        stdx::SpinLock mLock;
        AppenderList mAppenders;
        MessageQueue mQueue;

        /// @brief Number of messages that were dropped due to the queue being full.
        std::atomic<uint32_t> mDroppedCount{0};

        /// @brief Number of messages that were written out to the appenders.
        std::atomic<uint32_t> mComittedCount{0};

        void write(const LogMessageView& message) [[clang::nonreentrant]] REQUIRES(mLock);
        size_t writeAllMessages() REQUIRES(mLock);
    public:
        OsStatus addAppender(ILogAppender *appender) noexcept;
        void removeAppender(ILogAppender *appender) noexcept;

        OsStatus recordMessage(detail::LogMessage message) noexcept [[clang::reentrant]];
        OsStatus submit(detail::LogMessage message) noexcept [[clang::reentrant]];

        template<typename F>
        OsStatus submitImmediate(const Logger *logger, F&& func) noexcept [[clang::reentrant]] {
            struct QueueOutStream final : public IOutStream {
                LogQueue *mQueue;
                const Logger *mLogger;

                void write(stdx::StringView message) noexcept [[clang::reentrant]] override {
                    LogMessageView view {
                        .message = message,
                        .logger = mLogger,
                        .level = LogLevel::ePrint,
                    };

                    CLANG_DIAGNOSTIC_PUSH();
                    CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");
                    // Writing to the output is not reentrant, but we hold the lock here so it is safe
                    // to call.

                    mQueue->write(view);

                    CLANG_DIAGNOSTIC_POP();
                }

                QueueOutStream(const Logger *logger, LogQueue *queue) noexcept
                    : mQueue(queue)
                    , mLogger(logger)
                { }
            };

            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");
            // Formatting may not be reentrant, but only if the format function is not reentrant.
            // We control the output buffer and know that it is reentrant.

            QueueOutStream out { logger, this };
            if (mLock.try_lock()) {
                func(out);
                mLock.unlock();
                return OsStatusSuccess;
            } else {
                return OsStatusDeviceBusy;
            }

            CLANG_DIAGNOSTIC_POP();
        }

        size_t flush() [[clang::nonreentrant]];

        uint32_t getDroppedCount(std::memory_order order = std::memory_order_seq_cst) const noexcept {
            return mDroppedCount.load(order);
        }

        uint32_t getCommittedCount(std::memory_order order = std::memory_order_seq_cst) const noexcept {
            return mComittedCount.load(order);
        }

        OsStatus resizeQueue(uint32_t newCapacity) noexcept [[clang::allocating]];

        static OsStatus create(uint32_t messageQueueCapacity, LogQueue *queue) noexcept [[clang::allocating]];

        static void initGlobalLogQueue(uint32_t messageQueueCapacity);

        static constexpr LogQueue &getGlobalQueue() noexcept {
            return sLogQueue;
        }

        static OsStatus addGlobalAppender(ILogAppender *appender) {
            return getGlobalQueue().addAppender(appender);
        }

        static void removeGlobalAppender(ILogAppender *appender) noexcept {
            getGlobalQueue().removeAppender(appender);
        }
    };

    constinit inline LogQueue LogQueue::sLogQueue{};
}
