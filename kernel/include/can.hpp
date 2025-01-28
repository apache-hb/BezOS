#pragma once

#include "uart.hpp"

namespace km {
    /// @brief CANbus over serial.
    class CanBus {
        SerialPort *mSerialPort;

    public:
        constexpr CanBus(SerialPort *port)
            : mSerialPort(port)
        { }

        /// @brief Send a CAN message.
        ///
        /// @brief id the unique identifier for the message.
        /// @brief data the data to send.
        void sendDataFrame(uint32_t id, uint64_t data) {
            struct [[gnu::packed]] Packet {
                uint8_t start;
                uint32_t timestamp;
                uint8_t dlc;
                uint32_t id;
                uint64_t payload;
                uint8_t end;
            };

            Packet packet = {
                .start = 0xAA,
                .timestamp = 0,
                .dlc = 8,
                .id = id,
                .payload = data,
                .end = 0xBB,
            };

            mSerialPort->write(std::span(reinterpret_cast<uint8_t*>(&packet), sizeof(Packet)));
        }

        void sendDataFrame(uint32_t id, std::span<uint8_t> data) {
            mSerialPort->put(0xAA);
            uint32_t timestamp = 0;
            mSerialPort->write(std::span(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp)));
            mSerialPort->put(data.size_bytes());
            mSerialPort->write(std::span(reinterpret_cast<const uint8_t*>(&id), sizeof(id)));
            mSerialPort->write(data);
            mSerialPort->put(0xBB);
        }
    };
}
