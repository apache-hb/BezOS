#include "uart.hpp"

#include "delay.hpp"

#include "logger/logger.hpp"
#include <emmintrin.h>

constinit inline km::Logger UartLog { "UART" };

using namespace km::uart::detail;

bool km::SerialPort::waitForTransmit() noexcept [[clang::blocking, clang::nonallocating]] {
    uint8_t status = KmReadByte(mBasePort + kLineStatus);
    return status & kEmptyTransmit;
}

bool km::SerialPort::waitForReceive() noexcept [[clang::blocking, clang::nonallocating]] {
    uint8_t status = KmReadByte(mBasePort + kLineStatus);
    return status & kDataReady;
}

OsStatus km::SerialPort::put(uint8_t byte, unsigned timeout) noexcept [[clang::blocking, clang::nonallocating]] {
    while (!waitForTransmit()) {
        if (timeout-- == 0) {
            return OsStatusTimeout;
        }

        _mm_pause();
    }

    KmWriteByteNoDelay(mBasePort, byte);

    return OsStatusSuccess;
}

OsStatus km::SerialPort::get(uint8_t& byte, unsigned timeout) noexcept [[clang::blocking, clang::nonallocating]] {
    while (!waitForReceive()) {
        if (timeout-- == 0) {
            return OsStatusTimeout;
        }

        _mm_pause();
    }

    byte = KmReadByte(mBasePort);
    return OsStatusSuccess;
}

size_t km::SerialPort::write(std::span<const uint8_t> src, unsigned timeout) noexcept [[clang::blocking, clang::nonallocating]] {
    unsigned putTimeout = timeout;
    size_t i = 0;
    for (; i < src.size_bytes(); i++) {
        while (true) {
            OsStatus status = put(src[i], putTimeout);
            if (status == OsStatusSuccess)
                break;

            if (status != OsStatusTimeout)
                return i;

            if (timeout-- == 0)
                return i;
        }
    }

    return i;
}

size_t km::SerialPort::read(std::span<uint8_t> dst) noexcept [[clang::blocking, clang::nonallocating]] {
    size_t i = 0;
    for (; i < dst.size_bytes(); i++) {
        if (!get(dst[i])) {
            break;
        }
    }

    return i;
}

size_t km::SerialPort::print(stdx::StringView src, unsigned timeout) noexcept [[clang::blocking, clang::nonallocating]] {
    size_t result = 0;
    for (char c : src) {
        if (c == '\0')
            continue;

        if (c == '\n') {
            put('\r', timeout);
            result += 1;
        }

        put(c, timeout);
        result += 1;
    }

    return result;
}

OsStatus km::SerialPort::create(ComPortInfo info, SerialPort *port [[clang::noescape, gnu::nonnull]]) noexcept [[clang::blocking]] {
    uint16_t base = info.port;

    // do initial scratch test
    if (!info.skipScratchTest) {
        static constexpr uint8_t kScratchByte = 0x55;
        KmWriteByte(base + kScratch, kScratchByte);
        if (uint8_t read = KmReadByte(base + kScratch); read != kScratchByte) {
            UartLog.warnf("[", Hex(base), "] Scratch test failed ", Hex(read), " != ", Hex(kScratchByte));
            return OsStatusNotFound;
        }
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

            UartLog.warnf("[", Hex(base), "] Loopback test failed ", Hex(read), " != ", Hex(kLoopbackByte));
            return OsStatusDeviceFault;
        }

        // disable loopback
        KmWriteByte(base + kModemControl, 0x0F);
    }

    *port = SerialPort(info);
    return OsStatusSuccess;
}

km::OpenSerialResult km::OpenSerial(ComPortInfo info) {
    SerialPort port;
    OsStatus status = km::SerialPort::create(info, &port);
    switch (status) {
    case OsStatusSuccess:
        return { .port = port, .status = SerialPortStatus::eOk };
    case OsStatusNotFound:
        return { .status = SerialPortStatus::eScratchTestFailed };
    case OsStatusDeviceFault:
        return { .status = SerialPortStatus::eLoopbackTestFailed };
    default:
        return { .status = SerialPortStatus::eScratchTestFailed };
    }
}
