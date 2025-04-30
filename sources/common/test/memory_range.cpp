#include <gtest/gtest.h>

#include "common/range.hpp"
#include "common/physical_address.hpp"

using MemoryRange = sm::AnyRange<sm::PhysicalAddress>;

using namespace sm;

TEST(MemoryRangeTest, Contains) {
    MemoryRange range { 0x1000, 0x2000 };

    ASSERT_TRUE(range.contains(0x1000));
    ASSERT_TRUE(range.contains(0x1001));
    ASSERT_TRUE(range.contains(0x1FFF));
    ASSERT_FALSE(range.contains(0x2000));
    ASSERT_FALSE(range.contains(0x2001));
}

TEST(MemoryRangeTest, ContainsRange) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1100, 0x1200 };
    MemoryRange below = { 0x0000uz, 0x1100 };
    MemoryRange above = { 0x1F00, 0x3000 };

    ASSERT_TRUE(first.contains(second));
    ASSERT_FALSE(first.contains(below));
    ASSERT_FALSE(first.contains(above));
    ASSERT_FALSE(second.contains(first));
}

TEST(MemoryRangeTest, Size) {
    MemoryRange range { 0x1000, 0x2000 };

    ASSERT_EQ(range.size(), 0x1000);
}

TEST(MemoryRangeTest, Overlaps) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1500, 0x2500 };
    MemoryRange smaller = { 0x1100, 0x1200 };

    ASSERT_TRUE(first.overlaps(second));
    ASSERT_FALSE(first.overlaps(smaller));
}

TEST(MemoryRangeTest, OverlapIsCommutative) {
    MemoryRange first = { 0x100000, 0x200000 };
    MemoryRange second = { 0x100000, 0x7D47000 };

    ASSERT_TRUE(first.overlaps(second));
    ASSERT_TRUE(second.overlaps(first));
}

TEST(MemoryRangeTest, OverlapsEdge) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x2000, 0x3000 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST(MemoryRangeTest, OverlapsInner) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1800, 0x2000 };

    ASSERT_TRUE(first.overlaps(second));
}

TEST(MemoryRangeTest, OverlapsSubset) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1100, 0x1900 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST(MemoryRangeTest, NotOverlapping) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x3000, 0x4000 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST(MemoryRangeTest, Intersection) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1500, 0x2500 };
    MemoryRange smaller = { 0x1100, 0x1200 };
    MemoryRange noIntersection = { 0x3000, 0x4000 };

    {
        MemoryRange intersection = sm::intersection(first, second);
        ASSERT_EQ(intersection.front, second.front);
        ASSERT_EQ(intersection.back, first.back);
    }

    {
        MemoryRange intersection = sm::intersection(first, smaller);
        ASSERT_EQ(intersection.front, smaller.front);
        ASSERT_EQ(intersection.back, smaller.back);
    }

    {
        MemoryRange intersection = sm::intersection(first, noIntersection);
        ASSERT_EQ(intersection.front, 0);
        ASSERT_EQ(intersection.back, 0);
    }
}

static void TestRangeCut(MemoryRange first, MemoryRange second, MemoryRange expected) {
    MemoryRange result = first.cut(second);
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
    MemoryRange data = { 0xFFFF800000000000, 0xFFFFC00000000000 };
    MemoryRange committed = { 0xFFFF7FFFFF000000, 0xFFFF800000000000 };

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
        ASSERT_TRUE(outerAdjacent(a, b));
        ASSERT_TRUE(outerAdjacent(b, a));

        ASSERT_FALSE(outerAdjacent(b, c));
        ASSERT_FALSE(outerAdjacent(c, d));
    }
}

TEST(MemoryRangeTest, InnerAdjacent) {
    MemoryRange a = { 0x1000, 0x2000 };
    MemoryRange b = { 0x1500, 0x2000 };
    MemoryRange c = { 0x2000, 0x3000 };
    MemoryRange d = { 0x500, 0x1500 };

    {
        ASSERT_TRUE(innerAdjacent(a, b));
        ASSERT_TRUE(innerAdjacent(b, a));

        ASSERT_FALSE(innerAdjacent(b, c));
        ASSERT_FALSE(innerAdjacent(a, d));
    }
}

TEST(MemoryRangeTest, Contiguous) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x2000, 0x3000 };
    MemoryRange third = { 0x3000, 0x4000 };

    ASSERT_TRUE(contiguous(first, second));
    ASSERT_TRUE(contiguous(second, first));
    ASSERT_FALSE(contiguous(first, third));
    ASSERT_FALSE(contiguous(third, first));
}

TEST(MemoryRangeTest, ContiguousOverlapping) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1F00, 0x3000 };

    ASSERT_TRUE(contiguous(first, second));
    ASSERT_TRUE(contiguous(second, first));
}

TEST(MemoryRangeTest, IntervalAdjacent) {
    MemoryRange range = { 0x1000, 0x2000 };
    MemoryRange second = { 0x2000, 0x3000 };

    ASSERT_TRUE(interval(range, second));
}

TEST(MemoryRangeTest, IntervalOverlapping) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1F00, 0x3000 };

    ASSERT_TRUE(interval(first, second));
    ASSERT_TRUE(interval(second, first));
}

TEST(MemoryRangeTest, IntervalContains) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x1100, 0x1900 };

    ASSERT_TRUE(interval(first, second));
    ASSERT_TRUE(interval(second, first));
}

TEST(MemoryRangeTest, IntervalDisjoint) {
    MemoryRange first = { 0x1000, 0x2000 };
    MemoryRange second = { 0x3000, 0x4000 };

    ASSERT_FALSE(interval(first, second));
    ASSERT_FALSE(interval(second, first));
}

TEST(MemoryRangeTest, SplitAtAddress) {
    MemoryRange range = { 0x1000, 0x2000 };

    auto [first, second] = split(range, 0x1500);

    ASSERT_EQ(first.front.address, 0x1000);
    ASSERT_EQ(first.back.address, 0x1500);

    ASSERT_EQ(second.front.address, 0x1500);
    ASSERT_EQ(second.back.address, 0x2000);
}

TEST(MemoryRangeTest, SplitOnRange) {
    MemoryRange range = { 0x1000, 0x2000 };
    MemoryRange other = { 0x1500, 0x1800 };

    auto [first, second] = split(range, other);

    ASSERT_EQ(first.front.address, 0x1000);
    ASSERT_EQ(first.back.address, 0x1500);

    ASSERT_EQ(second.front.address, 0x1800);
    ASSERT_EQ(second.back.address, 0x2000);
}

TEST(MemoryRangeTest, SplitRangeBefore) {
    MemoryRange range = { 0x1000, 0x2000 };
    MemoryRange other = { 0x1100, 0x1500 };

    auto [first, second] = split(range, other);

    EXPECT_EQ(first.front.address, 0x1000);
    EXPECT_EQ(first.back.address, 0x1100);

    EXPECT_EQ(second.front.address, 0x1500);
    EXPECT_EQ(second.back.address, 0x2000);
}

TEST(MemoryRangeTest, Align) {
    MemoryRange range = { 0x1111, 0x3111 };
    MemoryRange aligned = sm::aligned(range, 0x1000);
    ASSERT_EQ(aligned.front.address, 0x2000);
    ASSERT_EQ(aligned.back.address, 0x3000);
}

TEST(MemoryRangeTest, AlignEmpty) {
    MemoryRange range = { 0x1111, 0x3111 };
    MemoryRange aligned = sm::aligned(range, 0x4000);
    ASSERT_TRUE(aligned.isEmpty());
}

static_assert(sm::rounddown(0x2111, 0x1000) == 0x2000);
static_assert(sm::roundup(0x1111, 0x1000) == 0x2000);
