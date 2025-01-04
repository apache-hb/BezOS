#pragma once

#include "util/format.hpp"

#include "std/span.hpp"

#include <stddef.h>
#include <stdint.h>

namespace km {
    struct ComPortInfo {
        uint16_t port;
        uint16_t divisor;
        uint8_t irq;

        bool skipLoopbackTest = false;
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

        bool put(uint8_t byte);

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
    ///
    /// @pre @a divisor is not 0
    OpenSerialResult openSerial(ComPortInfo info);
}

template<>
struct km::StaticFormat<km::SerialPortStatus> {
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
