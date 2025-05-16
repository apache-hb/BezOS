#pragma once

#include "logger/appender.hpp"

namespace km {
    class VgaAppender final : public ILogAppender {
        void write(const detail::LogMessage& message) override;
    };
}
