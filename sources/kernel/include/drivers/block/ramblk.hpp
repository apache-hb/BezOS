#pragma once

#include "drivers/block/driver.hpp"

namespace km {
    class MemoryBlk final : public km::IBlockDriver {
        static constexpr uint32_t kBlockSize = 512;

        std::byte *mMemory;
        size_t mSize;
        Protection mProtection;

        km::BlockDeviceStatus readImpl(uint64_t block, void *buffer, size_t count) override;
        km::BlockDeviceStatus writeImpl(uint64_t block, const void *buffer, size_t count) override;

    public:
        MemoryBlk(std::byte *memory, size_t size, Protection protection = Protection::eReadWrite);

        km::BlockDeviceCapability capability() const override;
    };
}
