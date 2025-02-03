#include <gtest/gtest.h>

#include "drivers/block/ramblk.hpp"

struct SectorRangeTest
    : public testing::TestWithParam<std::tuple<
        size_t, // offset
        size_t, // size
        km::BlockDeviceCapability, // capability
        km::detail::SectorRange // expected
    >> {};

static constexpr km::BlockDeviceCapability kDefaultBlock = {
    .protection = km::Protection::eReadWrite,
    .blockSize = 512,
    .blockCount = 0x10000,
};

INSTANTIATE_TEST_SUITE_P(SectorRangeTest, SectorRangeTest, testing::Values(
    std::make_tuple(0, 512, kDefaultBlock, km::detail::SectorRange {
        .firstSector = 0,
        .firstSectorOffset = 0,
        .lastSector = 0,
        .lastSectorOffset = 512,
    }),
    std::make_tuple(512, 512, kDefaultBlock, km::detail::SectorRange {
        .firstSector = 1,
        .firstSectorOffset = 0,
        .lastSector = 1,
        .lastSectorOffset = 512,
    }),
    std::make_tuple(512, 1024, kDefaultBlock, km::detail::SectorRange {
        .firstSector = 1,
        .firstSectorOffset = 0,
        .lastSector = 2,
        .lastSectorOffset = 512,
    }),
    std::make_tuple(512, 1023, kDefaultBlock, km::detail::SectorRange {
        .firstSector = 1,
        .firstSectorOffset = 0,
        .lastSector = 2,
        .lastSectorOffset = 511,
    }),
    std::make_tuple(512, 1025, kDefaultBlock, km::detail::SectorRange {
        .firstSector = 1,
        .firstSectorOffset = 0,
        .lastSector = 3,
        .lastSectorOffset = 1,
    }),
    std::make_tuple(100, 400, kDefaultBlock, km::detail::SectorRange {
        .firstSector = 0,
        .firstSectorOffset = 100,
        .lastSector = 0,
        .lastSectorOffset = 500,
    }),
));

TEST_P(SectorRangeTest, SectorRange) {
    auto [offset, size, capability, expected] = GetParam();
    auto range = km::detail::SectorRangeForSpan(offset, size, capability);

    EXPECT_EQ(range.firstSector, expected.firstSector);
    EXPECT_EQ(range.firstSectorOffset, expected.firstSectorOffset);
    EXPECT_EQ(range.lastSector, expected.lastSector);
    EXPECT_EQ(range.lastSectorOffset, expected.lastSectorOffset);

    EXPECT_EQ(range.firstSector * capability.blockSize + range.firstSectorOffset, offset);
    EXPECT_EQ(range.lastSector * capability.blockSize + range.lastSectorOffset, offset + size);
}

TEST(DriveMediaTest, ReadSector) {
    static constexpr size_t kSize = 0x10000;
    std::unique_ptr<std::byte[]> data{new std::byte[kSize]()};
    for (size_t i = 0; i < kSize; i++) {
        data[i] = std::byte(i % 256);
    }

    km::MemoryBlk ramblk(data.get(), kSize);
    km::BlockDevice media(&ramblk);

    ASSERT_EQ(media.size(), kSize);

    std::byte buffer[512];
    ASSERT_EQ(media.read(0, buffer, 512), 512);

    for (size_t i = 0; i < 512; i++) {
        ASSERT_EQ(buffer[i], std::byte(i));
    }
}
