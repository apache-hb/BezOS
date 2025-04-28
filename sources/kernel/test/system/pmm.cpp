#include <gtest/gtest.h>

#include "system/pmm.hpp"
#include "arch/paging.hpp"
#include "util/memory.hpp"

static constexpr km::MemoryRange kTestRange { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };

TEST(MemoryManagerInitTest, Construct) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);
}

class MemoryManagerTest : public testing::Test {
public:
    void SetUp() override {
        OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
        ASSERT_EQ(status, OsStatusSuccess);

        auto stats0 = manager.stats();
        ASSERT_EQ(stats0.segments, 0);
        ASSERT_EQ(stats0.heapStats.usedMemory, 0);
        ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size());
    }

    std::vector<km::MemoryRange> allocateMany(size_t count, size_t size) {
        std::vector<km::MemoryRange> ranges;
        ranges.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            km::MemoryRange range;
            OsStatus status = manager.allocate(size, alignof(x64::page), &range);
            EXPECT_EQ(status, OsStatusSuccess);
            EXPECT_EQ(range.size(), size);
            ranges.push_back(range);
        }

        std::sort(ranges.begin(), ranges.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.front < rhs.front;
        });

        return ranges;
    }

    void AssertSegmentNotFound(const km::MemoryRange& range) {
        AssertSegmentNotFound(range.front);
        AssertSegmentNotFound(range.back - 1);
        AssertSegmentNotFound((range.front.address + range.back.address) / 2);
    }

    void AssertSegmentNotFound(km::PhysicalAddress address) {
        sys2::MemorySegmentStats segmentStats;
        OsStatus status = manager.querySegment(address, &segmentStats);
        ASSERT_EQ(status, OsStatusNotFound);
    }

    void AssertSegmentFound(const km::MemoryRange& range, size_t owners) {
        AssertSegmentFound(range.front, range, owners);
        AssertSegmentFound(range.back - 1, range, owners);
        AssertSegmentFound((range.front.address + range.back.address) / 2, range, owners);
    }

    void AssertSegmentFound(km::PhysicalAddress address, const km::MemoryRange& range, size_t owners) {
        sys2::MemorySegmentStats segmentStats;
        OsStatus status = manager.querySegment(address, &segmentStats);
        ASSERT_EQ(status, OsStatusSuccess) << "Failed to find segment for address: " << std::string_view(km::format(address)) << " in range: " << std::string_view(km::format(range));
        ASSERT_EQ(segmentStats.owners, owners);
        ASSERT_EQ(segmentStats.range, range);
    }

    sys2::MemoryManagerStats AssertStats(size_t segments, size_t usedMemory) {
        auto stats = manager.stats();
        EXPECT_EQ(stats.segments, segments);
        EXPECT_EQ(stats.heapStats.usedMemory, usedMemory);
        EXPECT_EQ(stats.heapStats.freeMemory, kTestRange.size() - usedMemory);
        return stats;
    }

    sys2::MemoryManager manager;
};

TEST_F(MemoryManagerTest, Allocate) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    AssertStats(1, x64::kPageSize * 4);

    status = manager.release(range);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(0, 0);
}

TEST_F(MemoryManagerTest, ReleaseSubspan) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    AssertStats(1, x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front + x64::kPageSize, range.back - x64::kPageSize };
    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(2, x64::kPageSize * 2);

    auto [lhs, rhs] = km::split(range, subrange);
    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, lhs);

    AssertSegmentNotFound(subrange);
}

TEST_F(MemoryManagerTest, ReleaseHead) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    AssertStats(1, x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front, range.back - x64::kPageSize * 2 };
    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(1, x64::kPageSize * 2);

    AssertSegmentNotFound(subrange);
}

TEST_F(MemoryManagerTest, QuerySegment) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front, range.back - x64::kPageSize * 2 };
    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.back - 1, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, range.cut(subrange));

    sys2::MemorySegmentStats ss1;
    status = manager.querySegment(subrange.back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss1.owners, 2);
    EXPECT_EQ(ss1.range, subrange);

    sys2::MemorySegmentStats ss2;
    status = manager.querySegment(range.back, &ss2);
    EXPECT_EQ(status, OsStatusNotFound);

    sys2::MemorySegmentStats ss3;
    status = manager.querySegment(range.front - 1, &ss3);
    EXPECT_EQ(status, OsStatusNotFound);
}

TEST_F(MemoryManagerTest, ReleaseSpillFront) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front - x64::kPageSize, range.back - x64::kPageSize * 2 };
    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 1);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 2);

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.back - 1, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, range.cut(subrange));
}

TEST_F(MemoryManagerTest, ReleaseSpillBack) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front + x64::kPageSize, range.back + x64::kPageSize };
    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 1);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 1);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 1);

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, range.cut(subrange));
}

TEST_F(MemoryManagerTest, ReleaseSpill) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    status = manager.release({ range.front - x64::kPageSize, range.back + x64::kPageSize });
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 0);
    ASSERT_EQ(stats1.heapStats.usedMemory, 0);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size());
}

TEST_F(MemoryManagerTest, ReleaseTail) {
    km::MemoryRange range;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    status = manager.release({ range.front + x64::kPageSize * 2, range.back });
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 1);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 2);
}

TEST_F(MemoryManagerTest, ReleaseMultiple) {
    km::MemoryRange range0, range1;
    OsStatus status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range0.size(), x64::kPageSize * 4);

    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range1.size(), x64::kPageSize * 4);

    if (range0.front > range1.front) {
        std::swap(range0, range1);
    }

    km::MemoryRange subrange = { range0.front + x64::kPageSize * 2, range1.back - x64::kPageSize * 2 };

    AssertStats(2, x64::kPageSize * 4 * 2);

    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(2, x64::kPageSize * 2 * 2);
}

class MemoryManagerReleaseOneTest : public MemoryManagerTest {
public:
    void SetUp() override {
        MemoryManagerTest::SetUp();

        ranges = allocateMany(1, x64::kPageSize * 4);
        AssertStats(1, x64::kPageSize * 4);
        for (const auto& range : ranges) {
            AssertSegmentFound(range, 1);
        }
    }

    std::vector<km::MemoryRange> ranges;
};

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneExact) {
    km::MemoryRange subrange = ranges[0];
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(0, 0);

    AssertSegmentNotFound(ranges[0]);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneExactSpillLeft) {
    km::MemoryRange subrange = ranges[0].withExtraHead(x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(0, 0);

    AssertSegmentNotFound(ranges[0]);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneExactSpillRight) {
    km::MemoryRange subrange = ranges[0].withExtraTail(x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(0, 0);

    AssertSegmentNotFound(ranges[0]);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneExactSpillBoth) {
    km::MemoryRange subrange = ranges[0].withExtraTail(x64::kPageSize).withExtraTail(x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(0, 0);

    AssertSegmentNotFound(ranges[0]);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneSpillLeft) {
    km::MemoryRange subrange = ranges[0].offsetBy(-x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(1, x64::kPageSize);

    AssertSegmentNotFound(subrange);
    AssertSegmentFound(ranges[0].cut(subrange), 1);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneSpillRight) {
    km::MemoryRange subrange = ranges[0].offsetBy(x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(1, x64::kPageSize);

    AssertSegmentNotFound(subrange);
    AssertSegmentFound(ranges[0].cut(subrange), 1);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneRightBoundary) {
    km::MemoryRange subrange = ranges[0].offsetBy(ranges[0].size());
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusNotFound);

    AssertStats(1, x64::kPageSize * 4);

    AssertSegmentNotFound(subrange);
    AssertSegmentFound(ranges[0], 1);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneLeftBoundary) {
    km::MemoryRange subrange = ranges[0].offsetBy(-ranges[0].size());
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusNotFound);

    AssertStats(1, x64::kPageSize * 4);

    AssertSegmentNotFound(subrange);
    AssertSegmentFound(ranges[0], 1);
}

TEST_F(MemoryManagerReleaseOneTest, ReleaseOneInner) {
    km::MemoryRange subrange = ranges[0].subrange(x64::kPageSize, x64::kPageSize * 2);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(2, x64::kPageSize * 2);

    AssertSegmentNotFound(subrange);
    AssertSegmentFound(ranges[0].first(x64::kPageSize), 1);
    AssertSegmentFound(ranges[0].last(x64::kPageSize), 1);
}

class MemoryManagerReleaseManyTest : public MemoryManagerTest {
public:
    void SetUp() override {
        MemoryManagerTest::SetUp();

        ranges = allocateMany(3, x64::kPageSize * 4);
        AssertStats(3, x64::kPageSize * 4 * 3);
        for (const auto& range : ranges) {
            AssertSegmentFound(range, 1);
        }
    }

    std::vector<km::MemoryRange> ranges;
};

TEST_F(MemoryManagerReleaseManyTest, ReleaseManyInnerExact) {
    km::MemoryRange subrange = ranges[1];
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(2, x64::kPageSize * 4 * 2);

    AssertSegmentNotFound(ranges[1]);
    AssertSegmentFound(ranges[0], 1);
    AssertSegmentFound(ranges[2], 1);
}

TEST_F(MemoryManagerReleaseManyTest, ReleaseOuterRightBorder) {
    km::MemoryRange subrange = ranges[2].offsetBy(ranges[2].size());
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusNotFound);

    AssertStats(3, x64::kPageSize * 4 * 3);

    AssertSegmentFound(ranges[1], 1);
    AssertSegmentFound(ranges[0], 1);
    AssertSegmentFound(ranges[2], 1);
}

TEST_F(MemoryManagerReleaseManyTest, ReleaseOuterLeftBorder) {
    km::MemoryRange subrange = ranges[0].offsetBy(-ranges[0].size());
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusNotFound);

    AssertStats(3, x64::kPageSize * 4 * 3);

    AssertSegmentFound(ranges[1], 1);
    AssertSegmentFound(ranges[0], 1);
    AssertSegmentFound(ranges[2], 1);
}

TEST_F(MemoryManagerReleaseManyTest, ReleaseManyRightSpill) {
    km::MemoryRange subrange = ranges[2].offsetBy(x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(3, x64::kPageSize * ((4 * 2) + 1));

    AssertSegmentFound(ranges[1], 1);
    AssertSegmentFound(ranges[0], 1);
    AssertSegmentFound(ranges[2].cut(subrange), 1);
    AssertSegmentNotFound(subrange);
}

TEST_F(MemoryManagerReleaseManyTest, ReleaseManyLeftSpill) {
    km::MemoryRange subrange = ranges[0].offsetBy(-x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(3, x64::kPageSize * ((4 * 2) + 1));

    AssertSegmentNotFound(subrange);
    AssertSegmentFound(ranges[0].cut(subrange), 1);
    AssertSegmentFound(ranges[1], 1);
    AssertSegmentFound(ranges[2], 1);
}

TEST_F(MemoryManagerReleaseManyTest, ReleaseManyInnerSpill) {
    km::MemoryRange subrange = ranges[0].offsetBy(x64::kPageSize * 2);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(3, x64::kPageSize * (4 * 2));

    AssertSegmentNotFound(subrange);
    AssertSegmentFound(ranges[0].cut(subrange), 1);
    AssertSegmentFound(ranges[1].cut(subrange), 1);
    AssertSegmentFound(ranges[2], 1);
}

TEST_F(MemoryManagerReleaseManyTest, ReleaseAllLeftSpill) {
    km::MemoryRange subrange = unionInterval<km::PhysicalAddress>(ranges).withExtraHead(x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(0, 0);

    AssertSegmentNotFound(subrange);
    AssertSegmentNotFound(ranges[0]);
    AssertSegmentNotFound(ranges[1]);
    AssertSegmentNotFound(ranges[2]);
}

TEST_F(MemoryManagerReleaseManyTest, ReleaseAllRightSpill) {
    km::MemoryRange subrange = unionInterval<km::PhysicalAddress>(ranges).withExtraTail(x64::kPageSize);
    OsStatus status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    AssertStats(0, 0);

    AssertSegmentNotFound(subrange);
    AssertSegmentNotFound(ranges[0]);
    AssertSegmentNotFound(ranges[1]);
    AssertSegmentNotFound(ranges[2]);
}

TEST_F(MemoryManagerTest, ReleaseMany) {
    OsStatus status = OsStatusSuccess;
    std::array<km::MemoryRange, 3> ranges;
    for (auto& range : ranges) {
        status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
        EXPECT_EQ(status, OsStatusSuccess);
        EXPECT_EQ(range.size(), x64::kPageSize * 4);
    }

    std::sort(ranges.begin(), ranges.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.front < rhs.front;
    });

    km::MemoryRange subrange = { ranges.front().front + x64::kPageSize * 2, ranges.back().back - x64::kPageSize * 2 };

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, ranges.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * ranges.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * ranges.size());

    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2 * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 2 * 2);

    sys2::MemorySegmentStats ss0;
    km::MemoryRange ss0Range = { ranges.front().front, ranges.front().front + x64::kPageSize * 2 };
    status = manager.querySegment(ranges.front().front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.front().front.address << " failed";
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, ss0Range);

    sys2::MemorySegmentStats ss1;
    km::MemoryRange ss1Range = { ranges.back().back - x64::kPageSize * 2, ranges.back().back };
    status = manager.querySegment(ranges.back().back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.back().back.address << " failed";
    EXPECT_EQ(ss1.owners, 1);
    EXPECT_EQ(ss1.range, ss1Range);

    sys2::MemorySegmentStats ss2;
    status = manager.querySegment(ranges[1].front, &ss2);
    EXPECT_EQ(status, OsStatusNotFound);
}

TEST_F(MemoryManagerTest, RetainSingle) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    status = manager.retain(range);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 1);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size());
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size());

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 2);
    EXPECT_EQ(ss0.range, range);
}

TEST_F(MemoryManagerTest, RetainSpill) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front - x64::kPageSize, range.back + x64::kPageSize };
    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 1);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size());
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size());

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 2);
    EXPECT_EQ(ss0.range, range);
}

TEST_F(MemoryManagerTest, RetainFront) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front, range.back - x64::kPageSize * 2 };
    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size());
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size());

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 2);
    EXPECT_EQ(ss0.range, subrange);

    sys2::MemorySegmentStats ss1;
    status = manager.querySegment(range.back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss1.owners, 1);
    EXPECT_EQ(ss1.range, range.cut(subrange));
}

TEST_F(MemoryManagerTest, RetainBack) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front + x64::kPageSize * 2, range.back };
    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size());
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size());

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, range.cut(subrange));

    sys2::MemorySegmentStats ss1;
    status = manager.querySegment(range.back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss1.owners, 2);
    EXPECT_EQ(ss1.range, subrange);
}

TEST_F(MemoryManagerTest, RetainSubrange) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front + x64::kPageSize, range.back - x64::kPageSize };
    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 3);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size());
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size());

    auto [lhs, rhs] = km::split(range, subrange);
    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, lhs);

    sys2::MemorySegmentStats ss1;
    status = manager.querySegment(range.back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss1.owners, 1);
    EXPECT_EQ(ss1.range, rhs);

    sys2::MemorySegmentStats ss2;
    status = manager.querySegment(subrange.front, &ss2);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss2.owners, 2);
    EXPECT_EQ(ss2.range, subrange)
        << "range " << std::string_view(km::format(range))
        << " subrange " << std::string_view(km::format(subrange))
        << " ss2 " << std::string_view(km::format(ss2.range));
}

TEST_F(MemoryManagerTest, RetainSpillFront) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front - x64::kPageSize, range.back - x64::kPageSize * 2 };
    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size());
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size());

    km::MemoryRange subset = { range.front, subrange.back };
    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 2);
    EXPECT_EQ(ss0.range, subset)
        << "range " << std::string_view(km::format(range))
        << " subset " << std::string_view(km::format(subset))
        << " ss0 " << std::string_view(km::format(ss0.range));

    sys2::MemorySegmentStats ss1;
    status = manager.querySegment(range.back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss1.owners, 1);
    EXPECT_EQ(ss1.range, range.cut(subrange));
}

TEST_F(MemoryManagerTest, RetainSpillBack) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front + x64::kPageSize * 2, range.back + x64::kPageSize };
    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size());
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size());

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss0.owners, 1);
    EXPECT_EQ(ss0.range, range.cut(subrange));

    km::MemoryRange subset = { subrange.front, range.back };
    sys2::MemorySegmentStats ss1;
    status = manager.querySegment(range.back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss1.owners, 2);
    EXPECT_EQ(ss1.range, subset)
        << "range " << std::string_view(km::format(range))
        << " subset " << std::string_view(km::format(subset))
        << " ss1 " << std::string_view(km::format(ss1.range));
}

TEST_F(MemoryManagerTest, OverlapOps) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange retainSubrange = { range.front + x64::kPageSize, range.back };
    status = manager.retain(retainSubrange);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange releaseSubrange = { range.front, range.back - x64::kPageSize };
    status = manager.release(releaseSubrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, range.size() - x64::kPageSize);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - range.size() + x64::kPageSize);

    sys2::MemorySegmentStats ss0;
    status = manager.querySegment(range.front, &ss0);
    EXPECT_EQ(status, OsStatusNotFound);

    km::MemoryRange subset = { retainSubrange.front, releaseSubrange.back };
    sys2::MemorySegmentStats ss1;
    status = manager.querySegment(subset.back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss1.owners, 1);
    EXPECT_EQ(ss1.range, subset)
        << "range " << std::string_view(km::format(range))
        << " subset " << std::string_view(km::format(subset))
        << " ss1 " << std::string_view(km::format(ss1.range));

    sys2::MemorySegmentStats ss2;
    km::MemoryRange ss2Range = { releaseSubrange.back, retainSubrange.back };
    status = manager.querySegment(range.back - 1, &ss2);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(ss2.owners, 2);
    EXPECT_EQ(ss2.range, ss2Range)
        << "range " << std::string_view(km::format(range))
        << " ss2Range " << std::string_view(km::format(ss2Range))
        << " ss2 " << std::string_view(km::format(ss2.range));
}

TEST_F(MemoryManagerTest, RetainManyExactFront) {
    OsStatus status = OsStatusSuccess;
    std::array<km::MemoryRange, 3> ranges;
    for (auto& range : ranges) {
        status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
        EXPECT_EQ(status, OsStatusSuccess);
        EXPECT_EQ(range.size(), x64::kPageSize * 4);
    }

    std::sort(ranges.begin(), ranges.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.front < rhs.front;
    });

    km::MemoryRange subrange = { ranges.front().front, ranges.back().back - x64::kPageSize * 2 };

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, ranges.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * ranges.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * ranges.size());

    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 4); // one segment will be split by the retain
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 4 * 3);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * 3);

    sys2::MemorySegmentStats ss0;
    km::MemoryRange ss0Range = { ranges.front().front, ranges.front().back };
    status = manager.querySegment(ranges.front().front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.front().front.address << " failed";
    EXPECT_EQ(ss0.owners, 2);
    EXPECT_EQ(ss0.range, ss0Range);

    sys2::MemorySegmentStats ss1;
    km::MemoryRange ss1Range = { ranges.back().back - x64::kPageSize * 2, ranges.back().back };
    status = manager.querySegment(ranges.back().back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.back().back.address << " failed";
    EXPECT_EQ(ss1.owners, 1);
    EXPECT_EQ(ss1.range, ss1Range);

    sys2::MemorySegmentStats ss2;
    status = manager.querySegment(ranges[1].front, &ss2);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges[1].front.address << " failed";
    EXPECT_EQ(ss2.owners, 2);
}

TEST_F(MemoryManagerTest, RetainManyExactBack) {
    OsStatus status = OsStatusSuccess;
    std::array<km::MemoryRange, 3> ranges;
    for (auto& range : ranges) {
        status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
        EXPECT_EQ(status, OsStatusSuccess);
        EXPECT_EQ(range.size(), x64::kPageSize * 4);
    }

    std::sort(ranges.begin(), ranges.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.front < rhs.front;
    });

    km::MemoryRange subrange = { ranges.front().front + x64::kPageSize * 2, ranges.back().back };

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, ranges.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * ranges.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * ranges.size());

    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 4); // one segment will be split by the retain
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 4 * 3);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * 3);

    sys2::MemorySegmentStats ss0;
    km::MemoryRange ss0Range = { ranges.front().front + x64::kPageSize * 2, ranges.front().back };
    status = manager.querySegment(ranges.front().back - 1, &ss0);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.front().front.address << " failed";
    EXPECT_EQ(ss0.owners, 2);
    EXPECT_EQ(ss0.range, ss0Range) << " ss0Range " << std::string_view(km::format(ss0Range))
        << " ss0 " << std::string_view(km::format(ss0.range));

    sys2::MemorySegmentStats ss1;
    km::MemoryRange ss1Range = ranges.back();
    status = manager.querySegment(ranges.back().back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.back().back.address << " failed";
    EXPECT_EQ(ss1.owners, 2);
    EXPECT_EQ(ss1.range, ss1Range);

    sys2::MemorySegmentStats ss2;
    status = manager.querySegment(ranges[1].front, &ss2);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges[1].front.address << " failed";
    EXPECT_EQ(ss2.owners, 2);
}

TEST_F(MemoryManagerTest, RetainManyExactBoth) {
    OsStatus status = OsStatusSuccess;
    std::array<km::MemoryRange, 3> ranges;
    for (auto& range : ranges) {
        status = manager.allocate(x64::kPageSize * 4, alignof(x64::page), &range);
        EXPECT_EQ(status, OsStatusSuccess);
        EXPECT_EQ(range.size(), x64::kPageSize * 4);
    }

    std::sort(ranges.begin(), ranges.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.front < rhs.front;
    });

    km::MemoryRange subrange = { ranges.front().front, ranges.back().back };

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, ranges.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * ranges.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * ranges.size());

    status = manager.retain(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 3);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 4 * 3);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * 3);

    sys2::MemorySegmentStats ss0;
    km::MemoryRange ss0Range = { ranges.front().front, ranges.front().back };
    status = manager.querySegment(ranges.front().front, &ss0);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.front().front.address << " failed";
    EXPECT_EQ(ss0.owners, 2);
    EXPECT_EQ(ss0.range, ss0Range);

    sys2::MemorySegmentStats ss1;
    km::MemoryRange ss1Range = { ranges.back().front, ranges.back().back };
    status = manager.querySegment(ranges.back().back - 1, &ss1);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges.back().back.address << " failed";
    EXPECT_EQ(ss1.owners, 2);
    EXPECT_EQ(ss1.range, ss1Range) << " ss1Range " << std::string_view(km::format(ss1Range))
        << " ss1 " << std::string_view(km::format(ss1.range));

    sys2::MemorySegmentStats ss2;
    status = manager.querySegment(ranges[1].front, &ss2);
    EXPECT_EQ(status, OsStatusSuccess) << "Query for " << (void*)ranges[1].front.address << " failed";
    EXPECT_EQ(ss2.owners, 2) << " ss2 " << std::string_view(km::format(ss2.range))
        << " range " << std::string_view(km::format(ranges[1]));
}
