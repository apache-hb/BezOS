#include "logger/serial_appender.hpp"

#include "logger/logger.hpp"

void km::SerialAppender::write(const LogMessageView& message) {
    auto& [_, msg, logger, level] = message;

    if (level != LogLevel::ePrint) {
        mSerialPort.print("[");
        mSerialPort.print(logger->getName());
        mSerialPort.print("] ");
    }

    mSerialPort.print(msg);

    if (level != LogLevel::ePrint) {
        mSerialPort.put('\n');
    }
}

OsStatus km::SerialAppender::create(SerialPort port, SerialAppender *appender) noexcept {
    appender->mSerialPort = port;
    return OsStatusSuccess;
}
