#pragma once

#include "drivers/block/driver.hpp"

namespace km {
    class MemoryBlk final : public km::IBlockDevice {
        static constexpr uint32_t kBlockSize = 512;

        std::byte *mMemory;
        size_t mSize;
        Protection mProtection;
        bool mExternalMemory;

    public:
        MemoryBlk(size_t size);
        MemoryBlk(const std::byte *memory, size_t size);
        ~MemoryBlk();

        UTIL_NOCOPY(MemoryBlk);
        UTIL_NOMOVE(MemoryBlk);

        km::BlockDeviceCapability capability() const override;

        km::BlockDeviceStatus read(uint64_t block, void *buffer, size_t count) override;
        km::BlockDeviceStatus write(uint64_t block, const void *buffer, size_t count) override;
    };
}
