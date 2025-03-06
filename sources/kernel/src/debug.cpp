#include "debug/debug.hpp"

static constinit km::SerialPort gDebugSerialPort;

OsStatus km::debug::InitDebugStream(ComPortInfo info) {
    if constexpr (kEnableDebugEvents) {
        return OpenSerial(info, &gDebugSerialPort);
    }

    return OsStatusSuccess;
}

OsStatus km::debug::SendEvent(const EventPacket &packet) {
    if constexpr (kEnableDebugEvents) {
        if (!gDebugSerialPort.isReady()) {
            return OsStatusDeviceNotReady;
        }

        return gDebugSerialPort.write(packet);
    }

    return OsStatusSuccess;
}
