#include <gtest/gtest.h>

#include "memory/memory.hpp"

TEST(MemoryRangeTest, Contains) {
    km::MemoryRange range { 0x1000, 0x2000 };

    ASSERT_TRUE(range.contains(0x1000));
    ASSERT_TRUE(range.contains(0x1001));
    ASSERT_TRUE(range.contains(0x1FFF));
    ASSERT_FALSE(range.contains(0x2000));
    ASSERT_FALSE(range.contains(0x2001));
}

TEST(MemoryRangeTest, Size) {
    km::MemoryRange range { 0x1000, 0x2000 };

    ASSERT_EQ(range.size(), 0x1000);
}

TEST(MemoryRangeTest, Overlaps) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1500, 0x2500 };
    km::MemoryRange smaller = { 0x1100, 0x1200 };

    ASSERT_TRUE(first.overlaps(second));
    ASSERT_FALSE(first.overlaps(smaller));
}
