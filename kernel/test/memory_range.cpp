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
