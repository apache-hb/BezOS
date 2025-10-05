#pragma once

#include <stdint.h>

namespace km {
    class QemuExitDevice {
        uint16_t mIoBase;
        uint8_t mIoSize;

    public:
        QemuExitDevice(uint16_t ioBase = 0x501, uint8_t ioSize = 0x02) noexcept
            : mIoBase(ioBase)
            , mIoSize(ioSize)
        { }

        [[nodiscard]]
        uint16_t getIoBase() const noexcept { return mIoBase; }

        [[nodiscard]]
        uint8_t getIoSize() const noexcept { return mIoSize; }

        [[noreturn]]
        void exit(int code) const noexcept;
    };
}
