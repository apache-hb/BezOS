#include "test/fs/fs_test.hpp"

#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>

void FileBlk::cleanup() {
    if (mMemoryMap) {
        munmap(mMemoryMap, mMemorySize);
        mMemoryMap = nullptr;
    }

    if (mFileHandle >= 0) {
        close(mFileHandle);
        mFileHandle = -1;
    }
}

km::BlockDeviceStatus FileBlk::readImpl(uint64_t block, void *buffer, size_t count) {
    memcpy(buffer, (uint8_t*)mMemoryMap + (block * mBlockSize), count * mBlockSize);
    return km::BlockDeviceStatus::eOk;
}

km::BlockDeviceStatus FileBlk::writeImpl(uint64_t, const void *, size_t) {
    return km::BlockDeviceStatus::eReadOnly;
}

FileBlk::FileBlk(const std::filesystem::path& path, uint32_t blockSize)
    : mBlockSize(blockSize)
{
    mFileHandle = open(path.string().c_str(), O_RDONLY);
    if (mFileHandle < 0) {
        std::cerr << "Failed to open file: " << path << " = " << errno << std::endl;
        throw std::runtime_error("Failed to open file");
    }

    struct stat statbuf;
    if (fstat(mFileHandle, &statbuf) < 0) {
        std::cerr << "Failed to stat file: " << path << " = " << errno << std::endl;
        cleanup();
        throw std::runtime_error("Failed to stat file");
    }

    mMemorySize = statbuf.st_size;
    mMemoryMap = mmap(nullptr, mMemorySize, PROT_READ, MAP_PRIVATE, mFileHandle, 0);
    if (mMemoryMap == MAP_FAILED) {
        std::cerr << "Failed to mmap file: " << path << " = " << errno << std::endl;
        cleanup();
        throw std::runtime_error("Failed to mmap file");
    }
}

FileBlk::~FileBlk() {
    cleanup();
}

km::BlockDeviceCapability FileBlk::capability() const {
    return km::BlockDeviceCapability {
        .protection = km::Protection::eRead,
        .blockSize = mBlockSize,
        .blockCount = mMemorySize / mBlockSize,
    };
}
