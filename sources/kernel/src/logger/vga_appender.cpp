#include "logger/vga_appender.hpp"

#include "logger/logger.hpp"

void km::VgaAppender::write(const LogMessageView& message) {
    auto& [_, msg, logger, level] = message;

    if (level != LogLevel::ePrint) {
        mTerminal.print("[");
        mTerminal.print(logger->getName());
        mTerminal.print("] ");
    }

    mTerminal.print(msg);

    if (level != LogLevel::ePrint) {
        mTerminal.print("\n");
    }
}

OsStatus km::VgaAppender::create(boot::FrameBuffer fb, void *address, VgaAppender *appender) noexcept {
    appender->mTerminal = DirectTerminal(Canvas(fb, (uint8_t*)address));
    return OsStatusSuccess;
}
