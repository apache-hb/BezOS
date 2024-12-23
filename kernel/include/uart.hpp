#pragma once

#include "std/span.hpp"

#include <stddef.h>
#include <stdint.h>

namespace km {
    struct ComPortInfo {
        uint16_t port;
        uint8_t irq;
    };

    namespace com {
        static constexpr ComPortInfo kComPort1 = { .port = 0x3f8, .irq = 4 };
        static constexpr ComPortInfo kComPort2 = { .port = 0x2f8, .irq = 3 };
    }

    class SerialPort {
        static constexpr uint16_t kData = 0;
        static constexpr uint16_t kInterruptEnable = 1;
        static constexpr uint16_t kInterruptId = 2;
        static constexpr uint16_t kFifoControl = 2;
        static constexpr uint16_t kLineControl = 3;
        static constexpr uint16_t kModemControl = 4;
        static constexpr uint16_t kLineStatus = 5;
        static constexpr uint16_t kModemStatus = 6;
        static constexpr uint16_t kScratch = 7;

    public:
        SerialPort(ComPortInfo info);

        size_t write(stdx::Span<uint8_t> src);
        size_t read(stdx::Span<uint8_t> dst);
    };
}
