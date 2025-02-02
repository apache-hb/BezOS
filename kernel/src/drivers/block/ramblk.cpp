#include "drivers/block/ramblk.hpp"

km::BlockDeviceCapability km::MemoryBlk::capability() const {
    return km::BlockDeviceCapability {
        .protection = km::Protection::eReadWrite,
    };
}

km::BlockDeviceStatus km::MemoryBlk::read(uint64_t block, void *buffer, size_t count) {
    return km::BlockDeviceStatus::eInternalError;
}

km::BlockDeviceStatus km::MemoryBlk::write(uint64_t block, const void *buffer, size_t count) {
    return km::BlockDeviceStatus::eInternalError;
}
