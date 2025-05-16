#pragma once

#include "std/static_string.hpp"

#include <source_location>

namespace km {
    class Logger;
    class LogQueue;
    class ILogAppender;

    static constexpr size_t kLogMessageSize = 256;

    enum class LogLevel : uint8_t {
        eDebug = 0,
        eInfo = 1,
        eWarning = 2,
        eError = 3,
        eFatal = 4,
    };

    namespace detail {
        struct LogMessage {
            LogLevel level;
            std::source_location location;
            const Logger *logger;
            stdx::StaticString<kLogMessageSize> message;
        };
    }

    class ILogAppender {
    public:
        virtual ~ILogAppender() = default;

        virtual void write(const detail::LogMessage& message) [[clang::nonreentrant]] = 0;
    };
}
