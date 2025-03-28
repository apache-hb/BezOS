#include <gtest/gtest.h>
#include <mm_malloc.h>

#include "memory/range_allocator.hpp"

TEST(RangeDetailTest, MergeOverlapping) {
    stdx::Vector2<km::MemoryRange> ranges { };
    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x1500, 0x2500 });

    km::detail::MergeRanges(ranges);

    ASSERT_EQ(ranges.count(), 1);
    ASSERT_EQ(ranges[0].front, 0x1000);
    ASSERT_EQ(ranges[0].back, 0x2500);
}

TEST(RangeDetailTest, MergeContaining) {
    stdx::Vector2<km::MemoryRange> ranges { };
    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x1500, 0x1800 });

    km::detail::MergeRanges(ranges);

    ASSERT_EQ(ranges.count(), 1);
    ASSERT_EQ(ranges[0].front, 0x1000);
    ASSERT_EQ(ranges[0].back, 0x2000);
}

TEST(RangeDetailTest, MergeAdjacent) {
    stdx::Vector2<km::MemoryRange> ranges { };
    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x2000, 0x3000 });

    km::detail::MergeRanges(ranges);

    ASSERT_EQ(ranges.count(), 1);
    ASSERT_EQ(ranges[0].front, 0x1000);
    ASSERT_EQ(ranges[0].back, 0x3000);
}

TEST(RangeDetailTest, MultipleOverlapping) {
    stdx::Vector2<km::MemoryRange> ranges { };
    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x1500, 0x2500 });
    ranges.add({ 0x3000, 0x4000 });
    ranges.add({ 0x3500, 0x4500 });

    km::detail::MergeRanges(ranges);

    ASSERT_EQ(ranges.count(), 2);
    ASSERT_EQ(ranges[0].front, 0x1000);
    ASSERT_EQ(ranges[0].back, 0x2500);
    ASSERT_EQ(ranges[1].front, 0x3000);
    ASSERT_EQ(ranges[1].back, 0x4500);
}

TEST(RangeDetailTest, AllocateHintContains) {
    stdx::Vector2<km::MemoryRange> ranges {};
    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x3000, 0x4000 });

    km::MemoryRange hint { 0x1500, 0x2000 };
    km::MemoryRange range = km::detail::AllocateSpaceHint(ranges, 0x500, hint);
    ASSERT_EQ(range.front, 0x1500);
    ASSERT_EQ(range.back, 0x2000);
}

TEST(RangeDetailTest, AllocateHintSteps) {
    size_t align = 0x100;
    km::MemoryRange hint { 0x2000, 0x2400 };
    km::MemoryRange range0 = { 0x1000, 0x2000 };
    km::MemoryRange range1 = { 0x3000, 0x4000 };

    km::MemoryRange align0 = km::aligned(range0, align);
    ASSERT_EQ(align0.front, 0x1000);
    ASSERT_EQ(align0.back, 0x2000);

    km::MemoryRange align1 = km::aligned(range1, align);
    ASSERT_EQ(align1.front, 0x3000);
    ASSERT_EQ(align1.back, 0x4000);

    ASSERT_GE(align0.size(), hint.size());
    ASSERT_GE(align1.size(), hint.size());

    intptr_t distance0 = km::detail::FitDistance(align0, hint);
    ASSERT_EQ(distance0, -0x400);

    intptr_t distance1 = km::detail::FitDistance(align1, hint);
    ASSERT_EQ(distance1, 0x1000);
}

TEST(RangeDetailTest, AllocateHintBestFit) {
    stdx::Vector2<km::MemoryRange> ranges {};
    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x3000, 0x4000 });

    km::MemoryRange hint { 0x2000, 0x2400 };
    km::MemoryRange range = km::detail::AllocateSpaceHint(ranges, 0x100, hint);
    ASSERT_EQ(range.front.address, 0x1C00);
    ASSERT_EQ(range.back.address, 0x2000);
}

TEST(RangeDetailTest, AllocateHintNoFit) {
    stdx::Vector2<km::MemoryRange> ranges {};
    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x3000, 0x4000 });

    km::MemoryRange hint { 0x2000, 0x12500 };
    km::MemoryRange range = km::detail::AllocateSpaceHint(ranges, 0x500, hint);
    ASSERT_TRUE(range.isEmpty());
}

//
// Tests for km::detail::FitDistance can all be constexpr
//
#if 0
// other after range
static_assert(km::detail::FitDistance(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x2500, 0x3000 }) == -0x1000);

// other before range
static_assert(km::detail::FitDistance(km::MemoryRange { 0x2500, 0x4500 }, km::MemoryRange { 0x1000, 0x2000 }) == 0x1500);

// other after range + same size
static_assert(km::detail::FitDistance(km::MemoryRange { 0x2000, 0x3000 }, km::MemoryRange { 0x8000, 0x9000 }) == -0x6000);

// other before range + same size
static_assert(km::detail::FitDistance(km::MemoryRange { 0x8000, 0x9000 }, km::MemoryRange { 0x2000, 0x3000 }) == 0x6000);

// other before range + overlaps
static_assert(km::detail::FitDistance(km::MemoryRange { 0x2500, 0x4500 }, km::MemoryRange { 0x2000, 0x3000 }) == 0x500);

// other after range + overlaps
static_assert(km::detail::FitDistance(km::MemoryRange { 0x2000, 0x4000 }, km::MemoryRange { 0x2500, 0x4500 }) == -0x500);
#endif

static_assert(sm::magnitude(std::numeric_limits<intptr_t>::max()) == std::numeric_limits<intptr_t>::max());

class MergeRangeTest : public testing::TestWithParam<
    std::tuple<
        std::vector<km::MemoryRange>, /* input */
        std::vector<km::MemoryRange> /* output */
    >
> { };

INSTANTIATE_TEST_SUITE_P(MergeRange, MergeRangeTest, testing::Values(
    std::make_tuple( /* overlapping ranges */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x2500 },
        },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2500 },
        }
    ),
    std::make_tuple( /* containing ranges */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x1800 },
        },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
        }
    ),
    std::make_tuple( /* adjacent ranges */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x2000, 0x3000 },
        },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x3000 },
        }
    ),
    std::make_tuple( /* multiple adjacent */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x2500 },
            { 0x3000, 0x4000 },
            { 0x3500, 0x4500 },
        },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2500 },
            { 0x3000, 0x4500 },
        }
    ),
    std::make_tuple( /* no merging */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x3000, 0x4000 },
        },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x3000, 0x4000 },
        }
    ),
    std::make_tuple( /* empty */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x1000 },
        },
        std::vector<km::MemoryRange> {
            /* empty ranges should be removed */
        }
    ),
    std::make_tuple( /* mix of relations */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x1800 },
            { 0x3000, 0x4000 },
            { 0x3500, 0x4500 },
        },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x3000, 0x4500 },
        }
    ),
    std::make_tuple( /* overlapping sequence*/
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x2500 },
            { 0x2000, 0x3000 },
            { 0x2500, 0x3500 },
        },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x3500 },
        }
    )
));

TEST_P(MergeRangeTest, Merge) {
    auto [input, output] = GetParam();

    stdx::Vector2<km::MemoryRange> ranges { input.begin(), input.end() };

    km::detail::MergeRanges(ranges);

    ASSERT_EQ(ranges.count(), output.size());

    for (size_t i = 0; i < output.size(); i++) {
        ASSERT_EQ(ranges[i].front, output[i].front);
        ASSERT_EQ(ranges[i].back, output[i].back);
    }
}

class MarkUsedTest : public testing::TestWithParam<
    std::tuple<
        std::vector<km::MemoryRange>, /* input */
        km::MemoryRange, /* used */
        std::vector<km::MemoryRange> /* output */
    >
> { };

INSTANTIATE_TEST_SUITE_P(MarkUsed, MarkUsedTest, testing::Values(
    std::make_tuple( /* overlapping ranges */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x2500 },
        },
        km::MemoryRange { 0x1500, 0x2000 },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x1500 },
            { 0x2000, 0x2500 },
        }
    ),
    std::make_tuple( /* containing ranges */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x1800 },
        },
        km::MemoryRange { 0x1500, 0x1800 },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x1500 },
            { 0x1800, 0x2000 },
        }
    ),
    std::make_tuple( /* adjacent ranges */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x2000, 0x3000 },
        },
        km::MemoryRange { 0x2000, 0x2500 },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x2500, 0x3000 },
        }
    ),
    std::make_tuple( /* multiple adjacent */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x1500, 0x2500 },
            { 0x3000, 0x4000 },
            { 0x3500, 0x4500 },
        },
        km::MemoryRange { 0x1500, 0x2500 },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x1500 },
            { 0x3000, 0x4500 },
        }
    ),
    std::make_tuple( /* disjoint area */
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x3000, 0x4000 },
        },
        km::MemoryRange { 0x8000, 0x9000 },
        std::vector<km::MemoryRange> {
            { 0x1000, 0x2000 },
            { 0x3000, 0x4000 },
        }
    )
));

TEST_P(MarkUsedTest, MarkUsedArea) {
    auto [input, used, output] = GetParam();

    stdx::Vector2<km::MemoryRange> ranges { input.begin(), input.end() };

    km::detail::MergeRanges(ranges);

    km::detail::MarkUsedArea(ranges, used);

    km::detail::SortRanges(std::span(ranges));

    ASSERT_EQ(ranges.count(), output.size());

    for (size_t i = 0; i < output.size(); i++) {
        ASSERT_EQ(ranges[i].front, output[i].front);
        ASSERT_EQ(ranges[i].back, output[i].back);
    }
}
