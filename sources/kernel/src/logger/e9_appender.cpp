#include "logger/e9_appender.hpp"

#include "arch/intrin.hpp"

void km::E9Appender::write(const LogMessageView& message) {
    for (char c : message.message) {
        arch::Intrin::outbyte(kLogPort, c);
    }

    arch::Intrin::outbyte(kLogPort, '\n');
}

bool km::E9Appender::isAvailable() noexcept {
    return arch::Intrin::inbyte(kLogPort) == kLogPort;
}
