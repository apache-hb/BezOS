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

    status = manager.release({ range.front + x64::kPageSize, range.back - x64::kPageSize });
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 2);
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

    status = manager.release({ range.front, range.back - x64::kPageSize * 2 });
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats1 = manager.stats();
    ASSERT_EQ(stats1.segments, 1);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, kTestRange.size() - x64::kPageSize * 2);
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
