#pragma once

#include "std/inlined_vector.hpp"
#include "std/ringbuffer.hpp"
#include "std/shared_spinlock.hpp"
#include "std/static_string.hpp"
#include "util/format.hpp"

#include "logger/appender.hpp"

namespace km {
    class LogQueue {
        using MessageQueue = sm::AtomicRingQueue<detail::LogMessage>;
        using AppenderList = sm::InlinedVector<ILogAppender*, 4>;

        [[clang::no_destroy]]
        constinit static LogQueue GlobalLogQueue;

        stdx::SharedSpinLock mLock;
        AppenderList mAppenders;
        MessageQueue mQueue;

        /// @brief Number of messages that were dropped due to the queue being full.
        std::atomic<uint32_t> mDroppedCount{0};

        /// @brief Number of messages that were written out to the appenders.
        std::atomic<uint32_t> mComittedCount{0};

        void write(const detail::LogMessage& message) [[clang::nonreentrant]];
    public:
        OsStatus addAppender(ILogAppender *appender);
        void removeAppender(ILogAppender *appender) noexcept;

        OsStatus recordMessage(detail::LogMessage message) noexcept [[clang::reentrant]];
        OsStatus submit(detail::LogMessage message) noexcept [[clang::reentrant]];

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

        static constexpr LogQueue &getGlobalLogQueue() noexcept {
            return GlobalLogQueue;
        }

        static OsStatus addGlobalAppender(ILogAppender *appender) {
            return getGlobalLogQueue().addAppender(appender);
        }

        static void removeGlobalAppender(ILogAppender *appender) noexcept {
            getGlobalLogQueue().removeAppender(appender);
        }
    };

    constinit inline LogQueue LogQueue::GlobalLogQueue{};

    class Logger {
        LogQueue *mQueue;
        stdx::StringView mName;
    public:
        constexpr Logger() noexcept
            : Logger("DEFAULT")
        { }

        constexpr Logger(stdx::StringView name, LogQueue *queue) noexcept
            : mQueue(queue)
            , mName(name)
        { }

        constexpr Logger(stdx::StringView name) noexcept
            : Logger(name, &LogQueue::getGlobalLogQueue())
        { }

        stdx::StringView getName() const noexcept [[clang::reentrant]];

        void submit(LogLevel level, stdx::StringView message, std::source_location location) noexcept [[clang::reentrant]];

        template<typename... Args>
        void print(Args&&... args) noexcept [[clang::reentrant]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            submit(LogLevel::ePrint, message, std::source_location::current());
        }

        template<typename... Args>
        void println(Args&&... args) noexcept [[clang::reentrant]] {
            print(std::forward<Args>(args)..., "\n");
        }

        void dbg(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant]];
        void log(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant]];
        void warn(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant]];
        void error(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant]];
        void fatal(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant]];

        template<typename... Args>
        void dbgfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            dbg(message, location);
        }

        template<typename... Args>
        void logfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            log(message, location);
        }

        template<typename... Args>
        void warnfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            warn(message, location);
        }

        template<typename... Args>
        void errorfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            error(message, location);
        }

        template<typename... Args>
        void fatalfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            fatal(message, location);
        }
    };
}

#define dbgf(...) dbgfImpl(std::source_location::current(), __VA_ARGS__)
#define logf(...) logfImpl(std::source_location::current(), __VA_ARGS__)
#define warnf(...) warnfImpl(std::source_location::current(), __VA_ARGS__)
#define errorf(...) errorfImpl(std::source_location::current(), __VA_ARGS__)
#define fatalf(...) fatalfImpl(std::source_location::current(), __VA_ARGS__)
