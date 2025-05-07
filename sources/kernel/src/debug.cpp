#include "debug/debug.hpp"
#include "std/spinlock.hpp"

#if KM_DEBUG_EVENTS
static constinit km::SerialPort gDebugSerialPort;
static constinit stdx::SpinLock gDebugLock;
#endif

OsStatus km::debug::InitDebugStream([[maybe_unused]] ComPortInfo info) {
#if KM_DEBUG_EVENTS
    return OpenSerial(info, &gDebugSerialPort);
#else
    return OsStatusSuccess;
#endif
}

OsStatus km::debug::detail::SendEvent(const EventPacket &packet) noexcept [[clang::nonallocating]] {
#if KM_DEBUG_EVENTS
    if (!gDebugSerialPort.isReady()) {
        return OsStatusDeviceNotReady;
    }

    stdx::LockGuard guard(gDebugLock);

    return gDebugSerialPort.write(packet);
#else
    return OsStatusSuccess;
#endif
}
