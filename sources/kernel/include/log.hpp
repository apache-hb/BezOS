#pragma once

#include "boot.hpp"
#include "uart.hpp"

namespace km {
    enum class DebugLogLockType {
        eNone,
        eSpinLock,
    };

    void SetDebugLogLock(DebugLogLockType type);

    void InitDebugLog(std::span<boot::FrameBuffer> framebuffer);
    SerialPortStatus InitSerialDebugLog(ComPortInfo uart);

    void LockDebugLog();
    void UnlockDebugLog();
}
