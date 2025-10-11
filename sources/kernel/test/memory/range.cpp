#include <gtest/gtest.h>

#include "memory/range.hpp"

using namespace km;

class RangeTest : public testing::Test {
};

TEST_F(RangeTest, Contains) {
    km::MemoryRange range { 0x1000, 0x2000 };

    ASSERT_TRUE(range.contains(0x1000));
    ASSERT_TRUE(range.contains(0x1001));
    ASSERT_TRUE(range.contains(0x1FFF));
    ASSERT_FALSE(range.contains(0x2000));
    ASSERT_FALSE(range.contains(0x2001));
}

TEST_F(RangeTest, ContainsRange) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1100, 0x1200 };
    km::MemoryRange below = { 0x0000uz, 0x1100 };
    km::MemoryRange above = { 0x1F00, 0x3000 };

    ASSERT_TRUE(first.contains(second));
    ASSERT_FALSE(first.contains(below));
    ASSERT_FALSE(first.contains(above));
    ASSERT_FALSE(second.contains(first));
}

TEST_F(RangeTest, ContainsInnerAdjacent) {
    km::MemoryRange first = { 0x000000003FFFF000, 0x0000000040004000 };
    km::MemoryRange second = { 0x0000000040000000, 0x0000000040004000 };

    ASSERT_TRUE(first.contains(second));
}

TEST_F(RangeTest, Size) {
    km::MemoryRange range { 0x1000, 0x2000 };

    ASSERT_EQ(range.size(), 0x1000);
}

TEST_F(RangeTest, Overlaps) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1500, 0x2500 };
    km::MemoryRange smaller = { 0x1100, 0x1200 };

    ASSERT_TRUE(first.overlaps(second));
    ASSERT_FALSE(first.overlaps(smaller));
}

TEST_F(RangeTest, OverlapIsCommutative) {
    km::MemoryRange first = { 0x100000, 0x200000 };
    km::MemoryRange second = { 0x100000, 0x7D47000 };

    ASSERT_TRUE(first.overlaps(second));
    ASSERT_TRUE(second.overlaps(first));
}

TEST_F(RangeTest, OverlapsEdge) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x2000, 0x3000 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST_F(RangeTest, OverlapsInner) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1800, 0x2000 };

    ASSERT_TRUE(first.overlaps(second));
}

TEST_F(RangeTest, OverlapsSubset) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1100, 0x1900 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST_F(RangeTest, NotOverlapping) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x3000, 0x4000 };

    ASSERT_FALSE(first.overlaps(second));
}

TEST_F(RangeTest, Intersection) {
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

TEST_F(RangeTest, Cut) {
    // only the overlapping section is removed
    TestRangeCut({ 0x1000, 0x2000 }, { 0x1500, 0x2500 }, { 0x1000, 0x1500 });

    // when there is no overlap, the original range is returned
    TestRangeCut({ 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x1000, 0x2000 });

    // when the ranges only touch, the original range is returned
    TestRangeCut({ 0x1000, 0x2000 }, { 0x0000uz, 0x1000 }, { 0x1000, 0x2000 });

    TestRangeCut({ 0x100000, 0x200000 }, { 0x100000, 0x7D47000 }, { 0x200000, 0x7D47000 });

    TestRangeCut({ 0x100000, 0x7D47000 }, { 0x7D47000 - 0x100000, 0x7D47000 }, { 0x100000, 0x7D47000 - 0x100000 });
}

TEST_F(RangeTest, IntersectAtEdge) {
    km::MemoryRange data = { 0xFFFF800000000000, 0xFFFFC00000000000 };
    km::MemoryRange committed = { 0xFFFF7FFFFF000000, 0xFFFF800000000000 };

    ASSERT_TRUE(data.isValid());
    ASSERT_TRUE(data.isValid());

    ASSERT_FALSE(data.intersects(committed));
    ASSERT_FALSE(committed.intersects(data));
}

TEST_F(RangeTest, OuterAdjacent) {
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

TEST_F(RangeTest, InnerAdjacent) {
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

TEST_F(RangeTest, Contiguous) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x2000, 0x3000 };
    km::MemoryRange third = { 0x3000, 0x4000 };

    ASSERT_TRUE(km::contiguous(first, second));
    ASSERT_TRUE(km::contiguous(second, first));
    ASSERT_FALSE(km::contiguous(first, third));
    ASSERT_FALSE(km::contiguous(third, first));
}

TEST_F(RangeTest, ContiguousOverlapping) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1F00, 0x3000 };

    ASSERT_TRUE(km::contiguous(first, second));
    ASSERT_TRUE(km::contiguous(second, first));
}

TEST_F(RangeTest, IntervalAdjacent) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x2000, 0x3000 };

    ASSERT_TRUE(km::interval(range, second));
}

TEST_F(RangeTest, IntervalOverlapping) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1F00, 0x3000 };

    ASSERT_TRUE(km::interval(first, second));
    ASSERT_TRUE(km::interval(second, first));
}

TEST_F(RangeTest, IntervalContains) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x1100, 0x1900 };

    ASSERT_TRUE(km::interval(first, second));
    ASSERT_TRUE(km::interval(second, first));
}

TEST_F(RangeTest, IntervalDisjoint) {
    km::MemoryRange first = { 0x1000, 0x2000 };
    km::MemoryRange second = { 0x3000, 0x4000 };

    ASSERT_FALSE(km::interval(first, second));
    ASSERT_FALSE(km::interval(second, first));
}

TEST_F(RangeTest, SplitAtAddress) {
    km::MemoryRange range = { 0x1000, 0x2000 };

    auto [first, second] = km::split(range, 0x1500);

    ASSERT_EQ(first.front.address, 0x1000);
    ASSERT_EQ(first.back.address, 0x1500);

    ASSERT_EQ(second.front.address, 0x1500);
    ASSERT_EQ(second.back.address, 0x2000);
}

TEST_F(RangeTest, SplitOnRange) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange other = { 0x1500, 0x1800 };

    auto [first, second] = km::split(range, other);

    ASSERT_EQ(first.front.address, 0x1000);
    ASSERT_EQ(first.back.address, 0x1500);

    ASSERT_EQ(second.front.address, 0x1800);
    ASSERT_EQ(second.back.address, 0x2000);
}

TEST_F(RangeTest, SplitRangeBefore) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange other = { 0x1100, 0x1500 };

    auto [first, second] = km::split(range, other);

    EXPECT_EQ(first.front.address, 0x1000);
    EXPECT_EQ(first.back.address, 0x1100);

    EXPECT_EQ(second.front.address, 0x1500);
    EXPECT_EQ(second.back.address, 0x2000);
}

TEST_F(RangeTest, Align) {
    km::MemoryRange range = { 0x1111, 0x3111 };
    km::MemoryRange aligned = km::aligned(range, 0x1000);
    ASSERT_EQ(aligned.front.address, 0x2000);
    ASSERT_EQ(aligned.back.address, 0x3000);
}

static_assert(sm::rounddown(0x2111, 0x1000) == 0x2000);
static_assert(sm::roundup(0x1111, 0x1000) == 0x2000);

TEST_F(RangeTest, UnionIntervalEmpty) {
    std::vector<km::MemoryRange> ranges;
    km::MemoryRange result = km::combinedInterval<km::PhysicalAddress>(ranges);
    ASSERT_TRUE(result.isEmpty());
}

TEST_F(RangeTest, UnionIntervalSingle) {
    std::vector<km::MemoryRange> ranges = {
        km::MemoryRange { 0x1000, 0x2000 }
    };
    km::MemoryRange result = km::combinedInterval<km::PhysicalAddress>(ranges);
    ASSERT_EQ(result, ranges[0]);
}

TEST_F(RangeTest, UnionIntervalMultiple) {
    std::vector<km::MemoryRange> ranges = {
        km::MemoryRange { 0x1000, 0x2000 },
        km::MemoryRange { 0x4000, 0x5000 },
    };
    km::MemoryRange result = km::combinedInterval<km::PhysicalAddress>(ranges);
    ASSERT_EQ(result.front, 0x1000);
    ASSERT_EQ(result.back, 0x5000);
}

TEST_F(RangeTest, WithExtraHead) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange result = range.withExtraHead(0x1000);
    ASSERT_EQ(result.front.address, 0x0000);
    ASSERT_EQ(result.back.address, 0x2000);
}

TEST_F(RangeTest, WithExtraTail) {
    km::MemoryRange range = { 0x1000, 0x2000 };
    km::MemoryRange result = range.withExtraTail(0x1000);
    ASSERT_EQ(result.front.address, 0x1000);
    ASSERT_EQ(result.back.address, 0x3000);
}

TEST_F(RangeTest, StartsWith) {
    VirtualRangeEx range { (sm::VirtualAddress)0x1000, (sm::VirtualAddress)0x2000 };
    VirtualRangeEx other { (sm::VirtualAddress)0x1000, (sm::VirtualAddress)0x1800 };
    EXPECT_TRUE(range.startsWith(other));
    EXPECT_FALSE(other.startsWith(range));

    other = { (sm::VirtualAddress)0x1800, (sm::VirtualAddress)0x2000 };
    EXPECT_FALSE(range.startsWith(other));
    EXPECT_FALSE(other.startsWith(range));

    other = { (sm::VirtualAddress)0x0800, (sm::VirtualAddress)0x1800 };
    EXPECT_FALSE(range.startsWith(other));
    EXPECT_FALSE(other.startsWith(range));

    other = { (sm::VirtualAddress)0x0800, (sm::VirtualAddress)0x2800 };
    EXPECT_FALSE(range.startsWith(other));
    EXPECT_FALSE(other.startsWith(range));
}

TEST_F(RangeTest, EndsWith) {
    VirtualRangeEx range { (sm::VirtualAddress)0x1000, (sm::VirtualAddress)0x2000 };
    VirtualRangeEx other { (sm::VirtualAddress)0x1800, (sm::VirtualAddress)0x2000 };
    EXPECT_TRUE(range.endsWith(other));
    EXPECT_FALSE(other.endsWith(range));

    other = { (sm::VirtualAddress)0x1000, (sm::VirtualAddress)0x1800 };
    EXPECT_FALSE(range.endsWith(other));
    EXPECT_FALSE(other.endsWith(range));

    other = { (sm::VirtualAddress)0x0800, (sm::VirtualAddress)0x1800 };
    EXPECT_FALSE(range.endsWith(other));
    EXPECT_FALSE(other.endsWith(range));

    other = { (sm::VirtualAddress)0x0800, (sm::VirtualAddress)0x2800 };
    EXPECT_FALSE(range.endsWith(other));
    EXPECT_FALSE(other.endsWith(range));
}
