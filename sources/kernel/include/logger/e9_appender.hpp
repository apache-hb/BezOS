#pragma once

#include "logger/appender.hpp"

namespace km {
    class E9Appender final : public ILogAppender {
        void write(const detail::LogMessage& message) override;
    public:
        static constexpr uint16_t kLogPort = 0xE9;

        constexpr E9Appender() noexcept = default;

        static bool isAvailable() noexcept;
    };
}