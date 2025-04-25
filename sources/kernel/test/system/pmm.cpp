#include <gtest/gtest.h>

#include "system/pmm.hpp"
#include "arch/paging.hpp"
#include "util/memory.hpp"

static constexpr km::MemoryRange kTestRange { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };

TEST(MemoryManagerTest, Construct) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);
}

TEST(MemoryManagerTest, Allocate) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    status = manager.release(range);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 0);
    ASSERT_EQ(stats1.heapStats.usedMemory, 0);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size());
}

TEST(MemoryManagerTest, ReleaseSubspan) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front + x64::kPageSize, range.back - x64::kPageSize };
    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 2);

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
}

TEST(MemoryManagerTest, ReleaseHead) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range.size(), x64::kPageSize * 4);

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 1);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4);

    km::MemoryRange subrange = { range.front, range.back - x64::kPageSize * 2 };
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

TEST(MemoryManagerTest, ReleaseSpillFront) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, ReleaseSpillBack) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, ReleaseSpill) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, ReleaseTail) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, ReleaseMultiple) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range0, range1;
    status = manager.allocate(x64::kPageSize * 4, &range0);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range0.size(), x64::kPageSize * 4);

    status = manager.allocate(x64::kPageSize * 4, &range1);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_EQ(range1.size(), x64::kPageSize * 4);

    if (range0.front > range1.front) {
        std::swap(range0, range1);
    }

    km::MemoryRange subrange = { range0.front + x64::kPageSize * 2, range1.back - x64::kPageSize * 2 };

    auto stats0 = manager.stats();
    ASSERT_EQ(stats0.segments, 2);
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * 2);
    ASSERT_EQ(stats0.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 4 * 2);

    status = manager.release(subrange);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2 * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 2 * 2);
}

TEST(MemoryManagerTest, ReleaseMany) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    std::array<km::MemoryRange, 3> ranges;
    for (auto& range : ranges) {
        status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, RetainSingle) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, RetainSpill) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, RetainFront) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, RetainBack) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, RetainSubrange) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, RetainSpillFront) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, RetainSpillBack) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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

TEST(MemoryManagerTest, OverlapOps) {
    sys2::MemoryManager manager;
    OsStatus status = sys2::MemoryManager::create(kTestRange, &manager);
    EXPECT_EQ(status, OsStatusSuccess);

    km::MemoryRange range;
    status = manager.allocate(x64::kPageSize * 4, &range);
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
