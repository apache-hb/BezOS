#pragma once

#include "drivers/block/driver.hpp"

namespace km {
    class VirtIoBlk : public km::IBlockDevice {
        km::BlockDeviceCapability capability() const override;
        km::BlockDeviceStatus readImpl(uint64_t, void *, size_t) override;
        km::BlockDeviceStatus writeImpl(uint64_t, const void *, size_t) override;
    };
}
