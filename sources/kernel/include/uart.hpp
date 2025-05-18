#pragma once

#include <bezos/status.h>

#include "util/format.hpp"

#include <stddef.h>
#include <stdint.h>

namespace km {
    namespace uart::detail {
        // Offsets from the base serial port
        static constexpr uint16_t kData = 0;
        static constexpr uint16_t kInterruptEnable = 1;
        static constexpr uint16_t kFifoControl = 2;
        static constexpr uint16_t kLineControl = 3;
        static constexpr uint16_t kModemControl = 4;
        static constexpr uint16_t kLineStatus = 5;
        static constexpr uint16_t KModemStatus = 6;
        static constexpr uint16_t kScratch = 7;

        /// @brief Divisor Latch Access Bit
        static constexpr uint8_t kDlabOffset = 7;

        // Line status bits
        static constexpr uint8_t kEmptyTransmit = (1 << 5);
        static constexpr uint8_t kDataReady = (1 << 0);
    }

    struct ComPortInfo {
        uint16_t port;
        uint16_t divisor;
        uint8_t irq;

        bool skipLoopbackTest = false;
        bool skipScratchTest = false;
    };

    namespace com {
        static constexpr uint16_t kComPort1 = 0x3f8;
        static constexpr uint16_t kComPort2 = 0x2f8;
        static constexpr uint16_t kComPort3 = 0x3e8;
        static constexpr uint16_t kComPort4 = 0x2e8;
        static constexpr uint16_t kComPort5 = 0x5f8;
        static constexpr uint16_t kComPort6 = 0x4f8;
        static constexpr uint16_t kComPort7 = 0x5e8;
        static constexpr uint16_t kComPort8 = 0x4e8;

        static constexpr uint32_t kBaudRate = 115200;
        static constexpr uint16_t kBaud9600 = kBaudRate / 9600;
    }

    enum class SerialPortStatus {
        eOk,
        eScratchTestFailed,
        eLoopbackTestFailed,
    };

    class SerialPort {
        uint16_t mBasePort = 0xFFFF;
        uint8_t mIrq = 0xFF;

        /// @brief Returns true if the transmit buffer is empty.
        bool waitForTransmit() noexcept [[clang::blocking, clang::nonallocating]];

        /// @brief Returns true if the receive buffer has data.
        bool waitForReceive() noexcept [[clang::blocking, clang::nonallocating]];

        constexpr SerialPort(ComPortInfo info) noexcept
            : mBasePort(info.port)
            , mIrq(info.irq)
        { }

    public:
        constexpr SerialPort() noexcept = default;

        constexpr bool isReady() const noexcept { return mBasePort != 0xFFFF; }
        uint8_t irq() const noexcept { return mIrq; }

        size_t write(std::span<const uint8_t> src, unsigned timeout = 100) noexcept [[clang::blocking, clang::nonallocating]];
        size_t read(std::span<uint8_t> dst) noexcept [[clang::blocking, clang::nonallocating]];

        OsStatus put(uint8_t byte, unsigned timeout = 100) noexcept [[clang::blocking, clang::nonallocating]];
        OsStatus get(uint8_t& byte, unsigned timeout = 100) noexcept [[clang::blocking, clang::nonallocating]];

        size_t print(stdx::StringView src, unsigned timeout = 100) noexcept [[clang::blocking, clang::nonallocating]];

        template<typename T> requires (std::is_standard_layout_v<T>)
        OsStatus write(const T& src, unsigned timeout = 100) noexcept [[clang::blocking, clang::nonallocating]] {
            size_t result = write(std::span(reinterpret_cast<const uint8_t*>(&src), sizeof(T)), timeout);
            return result == sizeof(T) ? OsStatusSuccess : OsStatusTimeout;
        }

        static OsStatus create(ComPortInfo info, SerialPort *port [[clang::noescape, gnu::nonnull]]) noexcept [[clang::blocking]];
    };

    struct OpenSerialResult {
        SerialPort port;
        SerialPortStatus status;

        operator bool() const { return status != SerialPortStatus::eOk; }
    };

    /// @brief Construct a new Serial Port object
    ///
    /// @param info the port information
    ///
    /// @pre @a divisor is not 0
    OpenSerialResult OpenSerial(ComPortInfo info);
}

template<>
struct km::Format<km::SerialPortStatus> {
    static stdx::StringView toString(SerialPortStatus status) {
        switch (status) {
        case SerialPortStatus::eOk:
            return "Ok";
        case SerialPortStatus::eScratchTestFailed:
            return "Scratch Test Failed";
        case SerialPortStatus::eLoopbackTestFailed:
            return "Loopback Test Failed";
        }
    }
};
