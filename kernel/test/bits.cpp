#include <gtest/gtest.h>

#include "util/bits.hpp"

TEST(BitsTest, SetRangeSimple) {
    uint8_t data[16] = { 0 };
    sm::BitsSetRange(data, sm::BitCount(0), sm::BitCount(16));
    ASSERT_EQ(data[0], 0xFF);
    ASSERT_EQ(data[1], 0xFF);
    ASSERT_EQ(data[2], 0x00);
}

TEST(BitsTest, SetRangeCrossWord) {
    uint8_t data[16] = { 0 };
    sm::BitsSetRange(data, sm::BitCount(4), sm::BitCount(12));
    ASSERT_EQ(data[0], 0b1111'0000);
    ASSERT_EQ(data[1], 0b0000'1111);
    ASSERT_EQ(data[2], 0x00);
}

TEST(BitsTest, BitExtractSimple) {
    uint64_t value = 0x1234'5678'9ABC'DEF0;

    ASSERT_EQ(sm::BitExtract(value, 0, 16), 0xDEF0);
    ASSERT_EQ(sm::BitExtract(value, 16, 32), 0x9ABC);
    ASSERT_EQ(sm::BitExtract(value, 32, 48), 0x5678);
    ASSERT_EQ(sm::BitExtract(value, 48, 64), 0x1234);
}

TEST(BitsTest, BitMask) {
    ASSERT_EQ(sm::BitMask<uint64_t>(0, 16), 0xFFFF);
    ASSERT_EQ(sm::BitMask<uint64_t>(16, 32), 0xFFFF'0000);
    ASSERT_EQ(sm::BitMask<uint64_t>(32, 48), 0xFFFF'0000'0000);
    ASSERT_EQ(sm::BitMask<uint64_t>(48, 64), 0xFFFF'0000'0000'0000);

    ASSERT_EQ(sm::BitMask<uint64_t>(3, 8), 0b1111'1000);
    ASSERT_EQ(sm::BitMask<uint64_t>(4, 8), 0b1111'0000);
}
