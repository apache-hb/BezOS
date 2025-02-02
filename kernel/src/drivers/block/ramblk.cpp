#include "drivers/block/ramblk.hpp"

km::MemoryBlk::MemoryBlk(size_t size)
    : mMemory(new std::byte[size])
    , mSize(size)
    , mProtection(Protection::eReadWrite)
    , mExternalMemory(false)
{ }

km::MemoryBlk::MemoryBlk(const std::byte *memory, size_t size)
    : mMemory(const_cast<std::byte *>(memory))
    , mSize(size)
    , mProtection(Protection::eRead)
    , mExternalMemory(true)
{ }

km::MemoryBlk::~MemoryBlk() {
    if (mExternalMemory) return;

    delete[] mMemory;
}

km::BlockDeviceCapability km::MemoryBlk::capability() const {
    return km::BlockDeviceCapability {
        .protection = km::Protection::eReadWrite,
        .blockSize = kBlockSize,
        .blockCount = uint64_t(mSize / kBlockSize),
    };
}

km::BlockDeviceStatus km::MemoryBlk::read(uint64_t block, void *buffer, size_t count) {
    size_t front = (block * kBlockSize);
    size_t back = (block + count) * kBlockSize;
    if (back > mSize) {
        return km::BlockDeviceStatus::eOutOfRange;
    }

    std::memcpy(buffer, mMemory + front, count * kBlockSize);

    return km::BlockDeviceStatus::eOk;
}

km::BlockDeviceStatus km::MemoryBlk::write(uint64_t block, const void *buffer, size_t count) {
    if (!bool(mProtection & Protection::eWrite)) {
        return km::BlockDeviceStatus::eReadOnly;
    }

    size_t front = (block * kBlockSize);
    size_t back = (block + count) * kBlockSize;
    if (back > mSize) {
        return km::BlockDeviceStatus::eOutOfRange;
    }

    std::memcpy(mMemory + front, buffer, count * kBlockSize);
    return km::BlockDeviceStatus::eOk;
}
