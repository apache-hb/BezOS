#pragma once

#include "logger/appender.hpp"

#include "uart.hpp"

#include <bezos/status.h>

namespace km {
    class SerialAppender final : public ILogAppender {
        SerialPort mSerialPort;

        void write(const detail::LogMessage& message) override;

    public:
        constexpr SerialAppender() noexcept = default;

        static OsStatus create(SerialPort port, SerialAppender *appender) noexcept;
    };
}
