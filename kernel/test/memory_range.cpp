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

TEST(MemoryRangeTest, OverlapIsCommutative) {
    km::MemoryRange first = { 0x100000, 0x200000 };
    km::MemoryRange second = { 0x100000, 0x7D47000 };

    ASSERT_TRUE(first.overlaps(second));
    ASSERT_TRUE(second.overlaps(first));
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

static void TestRangeCut(km::MemoryRange first, km::MemoryRange second, km::MemoryRange expected) {
    km::MemoryRange result = first.cut(second);
    ASSERT_EQ(result.front.address, expected.front.address)
        << "Cut (" << first.front.address << ", " << first.back.address << ") with ("
        << second.front.address << ", " << second.back.address << ")";

    ASSERT_EQ(result.back.address, expected.back.address)
        << "Cut (" << first.front.address << ", " << first.back.address << ") with ("
        << second.front.address << ", " << second.back.address << ")";
}

TEST(MemoryRangeTest, Cut) {
    // only the overlapping section is removed
    TestRangeCut({ 0x1000, 0x2000 }, { 0x1500, 0x2500 }, { 0x1000, 0x1500 });

    // when there is no overlap, the original range is returned
    TestRangeCut({ 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x1000, 0x2000 });

    // when the ranges only touch, the original range is returned
    TestRangeCut({ 0x1000, 0x2000 }, { 0x0000uz, 0x1000 }, { 0x1000, 0x2000 });

    TestRangeCut({ 0x100000, 0x200000 }, { 0x100000, 0x7D47000 }, { 0x200000, 0x7D47000 });

    TestRangeCut({ 0x100000, 0x7D47000 }, { 0x7D47000 - 0x100000, 0x7D47000 }, { 0x100000, 0x7D47000 - 0x100000 });
}
