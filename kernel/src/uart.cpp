#include "uart.hpp"

#include "arch/intrin.hpp"
#include "kernel.hpp"

static constexpr uint8_t kDLABOffset = 7;

static constexpr uint16_t kData = 0;
static constexpr uint16_t kInterruptEnable = 1;
// static constexpr uint16_t kInterruptId = 2;
static constexpr uint16_t kFifoControl = 2;
static constexpr uint16_t kLineControl = 3;
static constexpr uint16_t kModemControl = 4;
static constexpr uint16_t kLineStatus = 5;
// static constexpr uint16_t kModemStatus = 6;
static constexpr uint16_t kScratch = 7;

void km::SerialPort::put(uint8_t byte) {
    while (!(__inbyte(mBasePort + kLineStatus) & 0x20)) { }
    __outbyte(mBasePort, byte);
}

size_t km::SerialPort::write(stdx::Span<const uint8_t> src) {
    for (uint8_t c : src) {
        put(c);
    }

    return src.sizeInBytes();
}

size_t km::SerialPort::read(stdx::Span<uint8_t> dst) {
    ssize_t i = 0;
    for (; i < dst.sizeInBytes(); i++) {
        if (!(__inbyte(mBasePort + kLineStatus) & 0x01)) {
            break;
        }

        dst[i] = __inbyte(mBasePort);
    }

    return i;
}

size_t km::SerialPort::print(stdx::StringView src) {
    size_t result = 0;
    for (char c : src) {
        if (c == '\n') {
            put('\r');
            result += 1;
        }

        put(c);
        result += 1;
    }

    return result;
}

km::OpenSerialResult km::openSerial(ComPortInfo info) {
    uint16_t base = info.port;

    // do initial scratch test
    static constexpr uint8_t kScratchByte = 0x55;
    __outbyte(base + kScratch, kScratchByte);
    if (uint8_t read = __inbyte(base + kScratch); read != kScratchByte) {
        KmDebugMessage("[UART][", Hex(base), "] Scratch test failed ", Hex(read), " != ", Hex(kScratchByte), "\n");
        return { .status = SerialPortStatus::eScratchTestFailed };
    }


    // disable interrupts
    __outbyte(base + kInterruptEnable, 0x00);

    // enable DLAB
    __outbyte(base + kLineControl, (1 << kDLABOffset));

    // set the divisor
    __outbyte(base + 0, (info.divisor & 0x00FF) >> 0);
    __outbyte(base + 1, (info.divisor & 0xFF00) >> 8);

    // disable DLAB and configure the line
    uint8_t lineControl
        = 0b11 // 8 bits
        | (0 << 2) // one stop bit
        | (0 << 3) // no parity bit
        | (0 << kDLABOffset); // DLAB off
    __outbyte(base + kLineControl, lineControl);

    // enable fifo
    uint8_t fifoControl
        = (1 << 0) // enable fifo
        | (1 << 1) // clear receive fifo
        | (1 << 2) // clear transmit fifo
        | (0b11 << 6); // 1 byte threshold
    __outbyte(base + kFifoControl, fifoControl);

    // enable irqs, rts/dtr set
    __outbyte(base + kModemControl, 0x0F);

    // do loopback test
    __outbyte(base + kModemControl, 0x1E);

    // send a byte and check if it comes back
    static constexpr uint8_t kLoopbackByte = 0xAE;
    __outbyte(base + kData, kLoopbackByte);
    if (uint8_t read = __inbyte(base + kData); read != kLoopbackByte) {
        KmDebugMessage("[UART][", Hex(base), "] Loopback test failed ", Hex(read), " != ", Hex(kLoopbackByte), "\n");
        return { .status = SerialPortStatus::eLoopbackTestFailed };
    }

    // disable loopback
    __outbyte(base + kModemControl, 0x0F);

    return { .port = SerialPort(info), .status = SerialPortStatus::eOk };
}
