#pragma once

#include "logger/appender.hpp"

namespace km {
    class SerialAppender final : public ILogAppender {
        void write(const detail::LogMessage& message) override;
    };
}
