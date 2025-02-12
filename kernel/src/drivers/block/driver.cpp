#include "drivers/block/driver.hpp"

km::BlockDeviceStatus km::IBlockDriver::read(uint64_t block, void *buffer, size_t count) {
    BlockDeviceCapability cap = capability();
    if (!bool(cap.protection & Protection::eRead)) {
        return BlockDeviceStatus::eReadOnly;
    }

    size_t back = (block + count);

    if (back > cap.blockCount) {
        return BlockDeviceStatus::eOutOfRange;
    }

    return readImpl(block, buffer, count);
}

km::BlockDeviceStatus km::IBlockDriver::write(uint64_t block, const void *buffer, size_t count) {
    BlockDeviceCapability cap = capability();
    if (!bool(cap.protection & Protection::eWrite)) {
        return BlockDeviceStatus::eReadOnly;
    }

    size_t back = (block + count);

    if (back > cap.blockCount) {
        return BlockDeviceStatus::eOutOfRange;
    }

    return writeImpl(block, buffer, count);
}

km::detail::SectorRange km::detail::SectorRangeForSpan(size_t offset, size_t size, const BlockDeviceCapability &cap) {
    // the first sector to load
    size_t firstSector = offset / cap.blockSize;
    // distance into the first sector to start reading from
    size_t firstSectorOffset = offset - (firstSector * cap.blockSize);

    // the last sector to load
    size_t lastSector = (offset + size) / cap.blockSize;
    // distance into the last sector to stop reading at
    size_t lastSectorOffset = (size + offset) - (lastSector * cap.blockSize);

    if (lastSectorOffset == 0) {
        lastSectorOffset = cap.blockSize;
        lastSector -= 1;
    }

    return SectorRange {
        .firstSector = firstSector,
        .firstSectorOffset = firstSectorOffset,
        .lastSector = lastSector,
        .lastSectorOffset = lastSectorOffset,
    };
}

km::BlockDevice::BlockDevice(IBlockDriver *device [[gnu::nonnull]])
    : mDevice(device)
    , mBuffer(new std::byte[size()])
{ }


// TODO: something in here is broken and reads too far

size_t km::BlockDevice::read(size_t offset, void *buffer, size_t size) {
    std::byte *dst = static_cast<std::byte*>(buffer);

    BlockDeviceCapability cap = mDevice->capability();
    if (offset + size > cap.size())
        return 0;

    auto [firstSector, firstSectorOffset, lastSector, lastSectorOffset] = detail::SectorRangeForSpan(offset, size, cap);

    // read in the first sector to the temp buffer
    if (mDevice->read(firstSector, mBuffer.get(), 1) != BlockDeviceStatus::eOk)
        return 0;

    // if the read is small then early exit here
    if (firstSector == lastSector) {
        size_t readSize = (lastSectorOffset - firstSectorOffset);
        memcpy(dst, mBuffer.get() + firstSectorOffset, readSize);
        return readSize;
    }

    size_t readSize = cap.blockSize - firstSectorOffset;
    memcpy(dst, mBuffer.get() + firstSectorOffset, readSize);

    // read any sectors after the first sector, not including the last sector
    size_t sectorCount = lastSector - firstSector - 1;
    if (sectorCount != 0) {
        if (mDevice->read(firstSector + 1, dst + firstSectorOffset, sectorCount) != BlockDeviceStatus::eOk)
            return readSize;

        readSize += (sectorCount * cap.blockSize);
    }

    // read in the last sector
    if (mDevice->read(lastSector, mBuffer.get(), 1) != BlockDeviceStatus::eOk)
        return readSize;

    size_t endSize = cap.blockSize - lastSectorOffset;
    memcpy(dst + readSize, mBuffer.get(), endSize);

    return readSize + endSize;
}

void km::BlockDevice::sync() {
    /* empty for now */
}

km::PartitionBlockDevice::PartitionBlockDevice(IBlockDriver *device, uint64_t front, uint64_t back)
    : mDevice(device)
    , mFront(front)
    , mBack(back)
{ }

km::BlockDeviceStatus km::PartitionBlockDevice::readImpl(uint64_t block, void *buffer, size_t count) {
    block += mFront;
    if (block + count > mBack) {
        return BlockDeviceStatus::eOutOfRange;
    }

    return mDevice->read(block, buffer, count);
}

km::BlockDeviceStatus km::PartitionBlockDevice::writeImpl(uint64_t block, const void *buffer, size_t count) {
    block += mFront;
    if (block + count > mBack) {
        return BlockDeviceStatus::eOutOfRange;
    }

    return mDevice->write(block, buffer, count);
}

km::BlockDeviceCapability km::PartitionBlockDevice::capability() const {
    BlockDeviceCapability cap = mDevice->capability();
    cap.blockCount = mBack - mFront;
    return cap;
}
