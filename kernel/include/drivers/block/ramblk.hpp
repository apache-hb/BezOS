#pragma once

#include "drivers/block/driver.hpp"

namespace km {
    class MemoryBlk final : public km::IBlockDevice {
        static constexpr uint32_t kBlockSize = 512;

        std::byte *mMemory;
        size_t mSize;
        Protection mProtection;

        km::BlockDeviceStatus readImpl(uint64_t block, void *buffer, size_t count) override;
        km::BlockDeviceStatus writeImpl(uint64_t block, const void *buffer, size_t count) override;

    public:
        MemoryBlk(const std::byte *memory, size_t size);

        km::BlockDeviceCapability capability() const override;
    };
}
