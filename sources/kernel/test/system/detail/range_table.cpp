#include <gtest/gtest.h>

#include "memory/range.hpp"
#include "system/detail/range_table.hpp"

struct TestSegment {
    km::MemoryRange inner;

    km::MemoryRange range() const noexcept {
        return inner;
    }
};

using RangeTable = sys::detail::RangeTable<TestSegment>;

class RangeTableTest : public testing::Test {
public:
    RangeTable table;

    km::MemoryRange insert(uintptr_t front, uintptr_t back) {
        km::MemoryRange range { front, back };
        TestSegment segment { range };
        table.insert(std::move(segment));
        return range;
    }
};

TEST_F(RangeTableTest, Insert) {
    auto range = insert(0x1000, 0x2000);

    auto it = table.find(0x1000);
    ASSERT_NE(it, table.end());
    EXPECT_EQ(it->second.range(), range);
}

class FindSingleRangeTest : public testing::TestWithParam<std::tuple<km::MemoryRange, km::MemoryRange, bool>> { };

INSTANTIATE_TEST_SUITE_P(
    FindSingleRange, FindSingleRangeTest,
    testing::Values(
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x1000, 0x2000 }, true),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x1100, 0x2000 }, true),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x1100, 0x2100 }, true),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x900, 0x2000 }, true),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x900, 0x1900 }, true),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x900, 0x2100 }, true),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x800, 0x900 }, false),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x2100, 0x2200 }, false),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x2000, 0x2200 }, false),
        std::make_tuple(km::MemoryRange { 0x1000, 0x2000 }, km::MemoryRange { 0x900, 0x1000 }, false),
        std::make_tuple(km::MemoryRange { 0x0000000040000000, 0x0000000040004000 }, km::MemoryRange { 0x0000000040001000, 0x0000000040003000 }, true)
    ));

TEST_P(FindSingleRangeTest, FindSingleRange) {
    auto [range, search, expected] = GetParam();
    RangeTable table;
    table.insert(TestSegment { range });

    auto [begin, end] = table.find(search);
    if (expected) {
        ASSERT_NE(begin, table.end()) << "Failed to find range: " << std::string_view(km::format(search)) << " in table with range: " << std::string_view(km::format(range));
        ASSERT_NE(end, table.end()) << "Failed to find range: " << std::string_view(km::format(search)) << " in table with range: " << std::string_view(km::format(range));
        ASSERT_EQ(begin, end) << "Failed to find range: " << std::string_view(km::format(search)) << " in table with range: " << std::string_view(km::format(range));

        ASSERT_EQ(begin->second.range(), range) << "begin: " << std::string_view(km::format(begin->second.range())) << " range: " << std::string_view(km::format(range));
        ASSERT_EQ(end->second.range(), range) << "end: " << std::string_view(km::format(end->second.range())) << " range: " << std::string_view(km::format(range));
    } else {
        EXPECT_EQ(begin, table.end());
        EXPECT_EQ(end, table.end());
    }
}

class FindMultipleRangeTest : public testing::TestWithParam<std::tuple<std::vector<km::MemoryRange>, km::MemoryRange, int, int, bool>> { };

INSTANTIATE_TEST_SUITE_P(
    FindMultipleRange, FindMultipleRangeTest,
    testing::Values(
        // |--a--| |--b--| |--c--|
        // |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 } },
            km::MemoryRange { 0x1000, 0x2000 },
            0, 0,
            true
        ),
        // |--a--| |--b--| |--c--|
        //         |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x3000, 0x4000 },
            1, 1,
            true
        ),
        // |--a--| |--b--| |--c--|
        //                 |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x5000, 0x6000 },
            2, 2,
            true
        ),
        //      |--a--| |--b--| |--c--|
        // |-s-|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x500, 0x800 },
            0, 0,
            false
        ),
        // |--a--|     |--b--| |--c--|
        //        |-s-|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x2500, 0x2800 },
            0, 0,
            false
        ),
        // |--a--| |--b--|     |--c--|
        //                |-s-|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x4500, 0x4800 },
            0, 0,
            false
        ),
        // |--a--| |--b--| |--c--|
        //                        |-s-|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x6200, 0x6800 },
            0, 0,
            false
        ),
        // |--a--| |--b--| |--c--|
        // |---s---|
        std::make_tuple(
            // 7
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x1000, 0x3000 },
            0, 0,
            true
        ),
        // |--a--| |--b--| |--c--|
        //   |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x1100, 0x3000 },
            0, 0,
            true
        ),
        //  |--a--| |--b--| |--c--|
        // |----s---|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x1000 - 0x100, 0x3000 },
            0, 0,
            true
        ),
        //  |--a--| |--b--| |--c--|
        // |----s----|
        std::make_tuple(
            // 10
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x1000 - 0x100, 0x2100 },
            0, 0,
            true
        ),
        //       |--a--| |--b--| |--c--|
        // |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x0000zu, 0x1000 },
            1, 1,
            false
        ),
        // |--a--|     |--b--| |--c--|
        //       |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x2000, 0x3000 },
            0, 0,
            false
        ),
        // |--a--| |--b--|     |--c--|
        //               |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x4000, 0x5000 },
            0, 0,
            false
        ),
        // |--a--| |--b--| |--c--|
        //                       |--s--|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x6000, 0x7000 },
            1, 1,
            false
        ),
        // |--a--| |--b--| |--c--|
        //       |---s---|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x2000, 0x4000 },
            1, 1,
            true
        ),
        // |--a--| |--b--| |--c--|
        //       |----s----|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x2000, 0x5000 },
            1, 1,
            true
        ),
        // |--a--| |--b--| |--c--|
        //       |-------s-------|
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x2000, 0x6000 },
            1, 2,
            true
        ),
        std::make_tuple(
            std::vector<km::MemoryRange> { { 0x1000, 0x2000 }, { 0x3000, 0x4000 }, { 0x5000, 0x6000 } },
            km::MemoryRange { 0x1000, 0x6000 },
            0, 2,
            true
        ),
        std::make_tuple(std::vector<km::MemoryRange> {}, km::MemoryRange {}, 0, 0, false)
    ));

TEST_P(FindMultipleRangeTest, FindRange) {
    auto [ranges, search, first, last, expected] = GetParam();
    RangeTable table;
    for (const auto& range : ranges) {
        table.insert(TestSegment { range });
    }

    auto [begin, end] = table.find(search);
    if (expected) {
        ASSERT_NE(begin, table.end()) << "Failed to find range: " << std::string_view(km::format(search));
        ASSERT_NE(end, table.end()) << "Failed to find range: " << std::string_view(km::format(search));

        ASSERT_EQ(begin->second.range(), ranges[first]) << "begin: " << std::string_view(km::format(begin->second.range())) << " first: " << std::string_view(km::format(ranges[first]));
        ASSERT_EQ(end->second.range(), ranges[last]) << "end: " << std::string_view(km::format(end->second.range())) << " last: " << std::string_view(km::format(ranges[last]));
    } else {
        EXPECT_EQ(begin, table.end());
        EXPECT_EQ(end, table.end());
    }
}

TEST_F(RangeTableTest, FindRangeEmpty) {
    auto [begin, end] = table.find({ 0x1000, 0x2000 });
    EXPECT_EQ(begin, table.end());
    EXPECT_EQ(end, table.end());
}
