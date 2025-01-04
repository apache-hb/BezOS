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

TEST(MemoryRangeTest, ContainsRange) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1100, 0x1200 };
    km::MemoryRange below = { 0x0000uz, 0x1100 };
    km::MemoryRange above = { 0x1F00, 0x3000 };

    ASSERT_TRUE(first.contains(second));
    ASSERT_FALSE(first.contains(below));
    ASSERT_FALSE(first.contains(above));
    ASSERT_FALSE(second.contains(first));
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

TEST(MemoryRangeTest, Intersection) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1500, 0x2500 };
    km::MemoryRange smaller = { 0x1100, 0x1200 };
    km::MemoryRange noIntersection = { 0x3000, 0x4000 };

    {
        km::MemoryRange intersection = km::intersection(first, second);
        ASSERT_EQ(intersection.front, second.front);
        ASSERT_EQ(intersection.back, first.back);
    }

    {
        km::MemoryRange intersection = km::intersection(first, smaller);
        ASSERT_EQ(intersection.front, smaller.front);
        ASSERT_EQ(intersection.back, smaller.back);
    }

    {
        km::MemoryRange intersection = km::intersection(first, noIntersection);
        ASSERT_EQ(intersection.front, 0);
        ASSERT_EQ(intersection.back, 0);
    }
}
