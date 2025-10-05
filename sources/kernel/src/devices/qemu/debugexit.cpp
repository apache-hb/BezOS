#include "devices/qemu/debugexit.hpp"

#include "panic.hpp"
#include "arch/intrin.hpp"

using km::QemuExitDevice;

void QemuExitDevice::exit(int code) const noexcept {
    switch (mIoSize) {
    case 0x01:
        arch::IntrinX86_64::outbyte(mIoBase, (uint8_t)code);
        break;
    case 0x02:
        arch::IntrinX86_64::outword(mIoBase, (uint16_t)code);
        break;
    case 0x04:
        arch::IntrinX86_64::outdword(mIoBase, (uint32_t)code);
        break;
    default:
        KM_PANIC("Unsupported QEMU debug exit port size.");
    }

    KmHalt();
}
