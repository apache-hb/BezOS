#pragma once

#include "logger/queue.hpp"

namespace km {
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
            : Logger(name, &LogQueue::getGlobalQueue())
        { }

        stdx::StringView getName() const noexcept [[clang::reentrant, clang::nonallocating]];

        void submit(LogLevel level, stdx::StringView message, std::source_location location) noexcept [[clang::reentrant, clang::nonallocating]];

        template<typename... Args>
        void print(Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            submit(LogLevel::ePrint, message, std::source_location::current());
        }

        template<typename... Args>
        void println(Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            print(std::forward<Args>(args)..., "\n");
        }

        template<typename... Args>
        OsStatus printImmediate(Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            return mQueue->submitImmediate(this, [&](IOutStream& out) noexcept [[clang::reentrant]] {
                CLANG_DIAGNOSTIC_PUSH();
                CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");
                // Formatting may not be reentrant, but only if the format function is not reentrant.
                // We control the output buffer and know that it is reentrant.

                out.format(std::forward<Args>(args)...);

                CLANG_DIAGNOSTIC_POP();
            });
        }

        template<typename... Args>
        void printlnImmediate(Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            printImmediate(std::forward<Args>(args)..., "\n");
        }

        void dbg(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant, clang::nonallocating]];
        void info(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant, clang::nonallocating]];
        void warn(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant, clang::nonallocating]];
        void error(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant, clang::nonallocating]];
        void fatal(stdx::StringView message, std::source_location location = std::source_location::current()) noexcept [[clang::reentrant, clang::nonallocating]];

        template<typename... Args>
        void dbgfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            dbg(message, location);
        }

        template<typename... Args>
        void infofImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            info(message, location);
        }

        template<typename... Args>
        void warnfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            warn(message, location);
        }

        template<typename... Args>
        void errorfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            error(message, location);
        }

        template<typename... Args>
        void fatalfImpl(std::source_location location, Args&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
            static_assert(sizeof...(Args) > 0, "No arguments provided");

            stdx::StaticString message = km::concat<kLogMessageSize>(std::forward<Args>(args)...);
            fatal(message, location);
        }
    };
}

#define dbgf(...) dbgfImpl(std::source_location::current(), __VA_ARGS__)
#define infof(...) infofImpl(std::source_location::current(), __VA_ARGS__)
#define warnf(...) warnfImpl(std::source_location::current(), __VA_ARGS__)
#define errorf(...) errorfImpl(std::source_location::current(), __VA_ARGS__)
#define fatalf(...) fatalfImpl(std::source_location::current(), __VA_ARGS__)
