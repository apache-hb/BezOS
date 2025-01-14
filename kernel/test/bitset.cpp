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
