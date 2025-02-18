#include "drivers/block/ramblk.hpp"

km::MemoryBlk::MemoryBlk(std::byte *memory, size_t size, Protection protection)
    : mMemory(memory)
    , mSize(size)
    , mProtection(protection)
{ }

km::BlockDeviceCapability km::MemoryBlk::capability() const {
    return km::BlockDeviceCapability {
        .protection = mProtection,
        .blockSize = kBlockSize,
        .blockCount = uint64_t(mSize / kBlockSize),
    };
}

km::BlockDeviceStatus km::MemoryBlk::readImpl(uint64_t block, void *buffer, size_t count) {
    size_t front = (block * kBlockSize);
    std::memcpy(buffer, mMemory + front, count * kBlockSize);
    return km::BlockDeviceStatus::eOk;
}

km::BlockDeviceStatus km::MemoryBlk::writeImpl(uint64_t block, const void *buffer, size_t count) {
    size_t front = (block * kBlockSize);
    std::memcpy(mMemory + front, buffer, count * kBlockSize);
    return km::BlockDeviceStatus::eOk;
}
