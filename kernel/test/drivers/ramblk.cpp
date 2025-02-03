#include <gtest/gtest.h>

#include "drivers/block/ramblk.hpp"

TEST(RamBlkTest, ReadSector) {
    static constexpr size_t kSize = 0x2000;
    std::unique_ptr<std::byte[]> data{new std::byte[kSize]()};
    for (size_t i = 0; i < kSize; i++) {
        data[i] = std::byte(i % 256);
    }

    km::MemoryBlk ramblk(data.get(), kSize);

    ASSERT_EQ(ramblk.capability().size(), kSize);

    std::unique_ptr<std::byte[]> buffer{new std::byte[ramblk.capability().blockSize]};

    ASSERT_EQ(ramblk.read(0, buffer.get(), 1), km::BlockDeviceStatus::eOk);

    for (size_t i = 0; i < ramblk.capability().blockSize; i++) {
        ASSERT_EQ(buffer[i], std::byte(i));
    }
}
