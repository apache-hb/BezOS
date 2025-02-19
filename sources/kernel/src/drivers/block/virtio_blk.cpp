#include "drivers/block/virtio.hpp"

km::BlockDeviceCapability km::VirtIoBlk::capability() const {
    return km::BlockDeviceCapability {
        .protection = km::Protection::eNone
    };
}

km::BlockDeviceStatus km::VirtIoBlk::readImpl(uint64_t, void *, size_t) {
    return km::BlockDeviceStatus::eInternalError;
}

km::BlockDeviceStatus km::VirtIoBlk::writeImpl(uint64_t, const void *, size_t) {
    return km::BlockDeviceStatus::eInternalError;
}
