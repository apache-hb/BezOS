#include "uart.hpp"

#include "delay.hpp"
#include "kernel.hpp"

using namespace km::uart::detail;

bool km::SerialPort::put(uint8_t byte) {
    if (!(KmReadByte(mBasePort + kLineStatus) & kEmptyTransmit)) {
        return false;
    }

    KmWriteByteNoDelay(mBasePort, byte);

    return true;
}

bool km::SerialPort::get(uint8_t& byte) {
    if (!(KmReadByte(mBasePort + kLineStatus) & kDataReady)) {
        return false;
    }

    byte = KmReadByte(mBasePort);
    return true;
}

size_t km::SerialPort::write(stdx::Span<const uint8_t> src) {
    size_t i = 0;
    for (; i < (size_t)src.sizeInBytes(); i++)
        if (!put(src[i]))
            break;

    return i;
}

size_t km::SerialPort::read(stdx::Span<uint8_t> dst) {
    size_t i = 0;
    for (; i < (size_t)dst.sizeInBytes(); i++)
        if (!get(dst[i]))
            break;

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

km::OpenSerialResult km::OpenSerial(ComPortInfo info) {
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
    KmWriteByte(base + kLineControl, (1 << kDlabOffset));

    // set the divisor
    KmWriteByte(base + 0, (info.divisor & 0x00FF) >> 0);
    KmWriteByte(base + 1, (info.divisor & 0xFF00) >> 8);

    // disable DLAB and configure the line
    uint8_t lineControl
        = 0b11 // 8 bits
        | (0 << 2) // one stop bit
        | (0 << 3) // no parity bit
        | (0 << kDlabOffset); // DLAB off
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

    if (!info.skipLoopbackTest) {
        // do loopback test
        KmWriteByte(base + kModemControl, 0x1E);
        static constexpr uint8_t kLoopbackByte = 0xAE;

        // send a byte and check if it comes back
        KmWriteByte(base + kData, kLoopbackByte);
        if (uint8_t read = KmReadByte(base + kData); read != kLoopbackByte) {
            KmWriteByte(base + kModemControl, 0x0F);

            KmDebugMessage("[UART][", Hex(base), "] Loopback test failed ", Hex(read), " != ", Hex(kLoopbackByte), "\n");
            return { .status = SerialPortStatus::eLoopbackTestFailed };
        }

        // disable loopback
        KmWriteByte(base + kModemControl, 0x0F);
    }

    return { .port = SerialPort(info), .status = SerialPortStatus::eOk };
}
