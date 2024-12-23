#pragma once

#include "util/format.hpp"

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

        void put(uint8_t byte);

    public:
        constexpr SerialPort() = default;

        /// @warning Do not use this constructor directly.
        ///          Use @ref openSerial instead.
        SerialPort(ComPortInfo info)
            : mBasePort(info.port)
        { }

        size_t write(stdx::Span<const uint8_t> src);
        size_t read(stdx::Span<uint8_t> dst);

        size_t print(stdx::StringView src);
    };

    struct OpenSerialResult {
        SerialPort port;
        SerialPortStatus status;

        operator bool() const { return status != SerialPortStatus::eOk; }
    };

    /// @brief Construct a new Serial Port object
    ///
    /// @param info the port information
    /// @param divisor the baud rate divisor
    ///
    /// @pre @a divisor is not 0
    OpenSerialResult openSerial(ComPortInfo info, uint16_t divisor);

    template<>
    struct StaticFormat<SerialPortStatus> {
        static constexpr size_t kStringSize = 16;
        static stdx::StringView toString(char*, SerialPortStatus status) {
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
}
