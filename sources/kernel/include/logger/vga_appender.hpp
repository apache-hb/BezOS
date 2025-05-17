#pragma once

#include "display.hpp"
#include "logger/appender.hpp"

#include "boot.hpp"

#include <bezos/status.h>

namespace km {
    class VgaAppender final : public ILogAppender {
        DirectTerminal mTerminal;

        void write(const detail::LogMessage& message) override;

    public:
        constexpr VgaAppender() noexcept = default;

        void setAddress(void *address) noexcept {
            mTerminal.setAddress(address);
        }

        static OsStatus create(boot::FrameBuffer fb, void *address, VgaAppender *appender) noexcept [[clang::allocating]];
    };
}
