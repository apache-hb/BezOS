#include "uart.hpp"

#include "delay.hpp"
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

static constexpr uint8_t kEmptyTransmit = (1 << 5);
static constexpr uint8_t kDataReady = (1 << 0);

bool km::SerialPort::put(uint8_t byte) {
    // TODO: should add some configuration for reliable transmission
    if (!(KmReadByte(mBasePort + kLineStatus) & kEmptyTransmit)) {
        return false;
    }

    KmWriteByteNoDelay(mBasePort, byte);

    return true;
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
        if (!(KmReadByte(mBasePort + kLineStatus) & kDataReady)) {
            break;
        }

        dst[i] = KmReadByte(mBasePort);
    }

    return i;
}

size_t km::SerialPort::print(stdx::StringView src) {
    size_t result = 0;
    for (char c : src) {
        if (c == '\0')
            continue;

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
    KmWriteByte(base + kScratch, kScratchByte);
    if (uint8_t read = KmReadByte(base + kScratch); read != kScratchByte) {
        KmDebugMessage("[UART][", Hex(base), "] Scratch test failed ", Hex(read), " != ", Hex(kScratchByte), "\n");
        return { .status = SerialPortStatus::eScratchTestFailed };
    }

    // disable interrupts
    KmWriteByte(base + kInterruptEnable, 0x00);

    // enable DLAB
    KmWriteByte(base + kLineControl, (1 << kDLABOffset));

    // set the divisor
    KmWriteByte(base + 0, (info.divisor & 0x00FF) >> 0);
    KmWriteByte(base + 1, (info.divisor & 0xFF00) >> 8);

    // disable DLAB and configure the line
    uint8_t lineControl
        = 0b11 // 8 bits
        | (0 << 2) // one stop bit
        | (0 << 3) // no parity bit
        | (0 << kDLABOffset); // DLAB off
    KmWriteByte(base + kLineControl, lineControl);

    // enable fifo
    uint8_t fifoControl
        = (1 << 0) // enable fifo
        | (1 << 1) // clear receive fifo
        | (1 << 2) // clear transmit fifo
        | (0b11 << 6); // 1 byte threshold
    KmWriteByte(base + kFifoControl, fifoControl);

    // enable irqs, rts/dtr set
    KmWriteByte(base + kModemControl, 0x0F);

    // do loopback test
    KmWriteByte(base + kModemControl, 0x1E);

    // send a byte and check if it comes back
    if (!info.skipLoopbackTest) {
        static constexpr uint8_t kLoopbackByte = 0xAE;
        KmWriteByte(base + kData, kLoopbackByte);
        if (uint8_t read = KmReadByte(base + kData); read != kLoopbackByte) {
            KmDebugMessage("[UART][", Hex(base), "] Loopback test failed ", Hex(read), " != ", Hex(kLoopbackByte), "\n");
            return { .status = SerialPortStatus::eLoopbackTestFailed };
        }
    }

    // disable loopback
    KmWriteByte(base + kModemControl, 0x0F);

    return { .port = SerialPort(info), .status = SerialPortStatus::eOk };
}
