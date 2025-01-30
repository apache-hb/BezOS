#include "drivers/block/driver.hpp"

class VirtioBlk : public km::IBlockDevice {
    km::BlockDeviceStatus getStatus() const override {
        return km::BlockDeviceStatus { .readSupport = false, .writeSupport = false };
    }
};

DEVICE_DRIVER(kVirtioBlk) = {
    .vendorId = pci::VendorId::eQemuVirtio,
    .deviceId = pci::DeviceId(0x1001),
    .size = sizeof(VirtioBlk),
    .align = alignof(VirtioBlk),
    .factory = [](void *memory) -> km::IBlockDevice* {
        return new (memory) VirtioBlk();
    },
};
