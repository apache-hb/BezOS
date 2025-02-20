#include <gtest/gtest.h>
#include <mm_malloc.h>

#include "memory/range_allocator.hpp"

static constexpr size_t kSize = 0x10000;

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
