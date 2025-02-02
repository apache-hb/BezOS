#include "drivers/block/driver.hpp"

class VirtIoBlk : public km::IBlockDevice {
    km::BlockDeviceCapability capability() const override {
        return km::BlockDeviceCapability {
            .protection = km::Protection::eNone
        };
    }

    km::BlockDeviceStatus read(uint64_t block, void *buffer, size_t count) override {
        return km::BlockDeviceStatus::eInternalError;
    }

    km::BlockDeviceStatus write(uint64_t block, const void *buffer, size_t count) override {
        return km::BlockDeviceStatus::eInternalError;
    }
};

DEVICE_DRIVER(kVirtioBlk) = {
    .vendorId = pci::VendorId::eQemuVirtio,
    .deviceId = pci::DeviceId(0x1001),
    .size = sizeof(VirtIoBlk),
    .align = alignof(VirtIoBlk),
    .factory = [](void *memory) -> km::IBlockDevice* {
        return new (memory) VirtIoBlk();
    },
};

km::IBlockDevice *VirtIoBlockDevice() {
    return new VirtIoBlk();
}
