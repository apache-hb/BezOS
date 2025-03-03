#include <gtest/gtest.h>

#include "memory/range.hpp"

using namespace km;

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

TEST(MemoryRangeTest, OverlapsEdge) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x2000, 0x3000 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST(MemoryRangeTest, OverlapsInner) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1800, 0x2000 };

    ASSERT_TRUE(first.overlaps(second));
}

TEST(MemoryRangeTest, OverlapsSubset) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1100, 0x1900 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST(MemoryRangeTest, NotOverlapping) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x3000, 0x4000 };

    ASSERT_FALSE(first.overlaps(second));
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

TEST(MemoryRangeTest, IntersectAtEdge) {
    km::MemoryRange data = { 0xFFFF800000000000, 0xFFFFC00000000000 };
    km::MemoryRange committed = { 0xFFFF7FFFFF000000, 0xFFFF800000000000 };

    ASSERT_TRUE(data.isValid());
    ASSERT_TRUE(data.isValid());

    ASSERT_FALSE(data.intersects(committed));
    ASSERT_FALSE(committed.intersects(data));
}

TEST(MemoryRangeTest, OuterAdjacent) {
    MemoryRange a = { 0x1000, 0x2000 };
    MemoryRange b = { 0x2000, 0x3000 };
    MemoryRange c = { 0x2500, 0x3000 };
    MemoryRange d = { 0x2500, 0x3500 };

    {
        ASSERT_TRUE(km::outerAdjacent(a, b));
        ASSERT_TRUE(km::outerAdjacent(b, a));

        ASSERT_FALSE(km::outerAdjacent(b, c));
        ASSERT_FALSE(km::outerAdjacent(c, d));
    }
}

TEST(MemoryRangeTest, InnerAdjacent) {
    MemoryRange a = { 0x1000, 0x2000 };
    MemoryRange b = { 0x1500, 0x2000 };
    MemoryRange c = { 0x2000, 0x3000 };
    MemoryRange d = { 0x500, 0x1500 };

    {
        ASSERT_TRUE(km::innerAdjacent(a, b));
        ASSERT_TRUE(km::innerAdjacent(b, a));

        ASSERT_FALSE(km::innerAdjacent(b, c));
        ASSERT_FALSE(km::innerAdjacent(a, d));
    }
}

TEST(MemoryRangeTest, Contiguous) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x2000, 0x3000 };
    km::MemoryRange third = { 0x3000, 0x4000 };

    ASSERT_TRUE(km::contiguous(first, second));
    ASSERT_TRUE(km::contiguous(second, first));
    ASSERT_FALSE(km::contiguous(first, third));
    ASSERT_FALSE(km::contiguous(third, first));
}

TEST(MemoryRangeTest, ContiguousOverlapping) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1F00, 0x3000 };

    ASSERT_TRUE(km::contiguous(first, second));
    ASSERT_TRUE(km::contiguous(second, first));
}

TEST(MemoryRangeTest, IntervalAdjacent) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x2000, 0x3000 };

    ASSERT_TRUE(km::interval(range, second));
}

TEST(MemoryRangeTest, IntervalOverlapping) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1F00, 0x3000 };

    ASSERT_TRUE(km::interval(first, second));
    ASSERT_TRUE(km::interval(second, first));
}

TEST(MemoryRangeTest, IntervalContains) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1100, 0x1900 };

    ASSERT_TRUE(km::interval(first, second));
    ASSERT_TRUE(km::interval(second, first));
}

TEST(MemoryRangeTest, IntervalDisjoint) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x3000, 0x4000 };

    ASSERT_FALSE(km::interval(first, second));
    ASSERT_FALSE(km::interval(second, first));
}

TEST(MemoryRangeTest, SplitAtAddress) {
    km::MemoryRange range = { 0x1000, 0x2000 };

    auto [first, second] = km::split(range, 0x1500);

    ASSERT_EQ(first.front.address, 0x1000);
    ASSERT_EQ(first.back.address, 0x1500);

    ASSERT_EQ(second.front.address, 0x1500);
    ASSERT_EQ(second.back.address, 0x2000);
}

TEST(MemoryRangeTest, SplitOnRange) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange other = { 0x1500, 0x1800 };

    auto [first, second] = km::split(range, other);

    ASSERT_EQ(first.front.address, 0x1000);
    ASSERT_EQ(first.back.address, 0x1500);

    ASSERT_EQ(second.front.address, 0x1800);
    ASSERT_EQ(second.back.address, 0x2000);
}

TEST(MemoryRangeTest, SplitRangeBefore) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange other = { 0x1100, 0x1500 };

    auto [first, second] = km::split(range, other);

    EXPECT_EQ(first.front.address, 0x1000);
    EXPECT_EQ(first.back.address, 0x1100);

    EXPECT_EQ(second.front.address, 0x1500);
    EXPECT_EQ(second.back.address, 0x2000);
}

TEST(MemoryRangeTest, Align) {
    km::MemoryRange range = { 0x1111, 0x3111 };
    km::MemoryRange aligned = km::aligned(range, 0x1000);
    ASSERT_EQ(aligned.front.address, 0x2000);
    ASSERT_EQ(aligned.back.address, 0x3000);
}

static_assert(sm::rounddown(0x2111, 0x1000) == 0x2000);
static_assert(sm::roundup(0x1111, 0x1000) == 0x2000);
