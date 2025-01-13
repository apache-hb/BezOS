#include "drivers/driver.hpp"

DEVICE_DRIVER(kVirtioBlk) = {
    .vendorId = pci::VendorId::eQemuVirtio,
    .deviceId = pci::DeviceId(0x1001),
};
