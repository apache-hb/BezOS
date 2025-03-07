#include "debug/debug.hpp"
#include "std/spinlock.hpp"

static constinit km::SerialPort gDebugSerialPort;
static constinit stdx::SpinLock gDebugLock;

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

        stdx::LockGuard guard(gDebugLock);

        return gDebugSerialPort.write(packet);
    }

    return OsStatusSuccess;
}
