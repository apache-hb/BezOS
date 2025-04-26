#include <gtest/gtest.h>

#include "memory/heap_command_list.hpp"
#include "memory/heap.hpp"
#include "test/new_shim.hpp"

class TlsfCommandListTest : public testing::Test {
public:
    void SetUp() override {
        GetGlobalAllocator()->reset();
    }
};

TEST_F(TlsfCommandListTest, Construct) {
    km::TlsfHeap heap;
    km::MemoryRange range { 0x1000, 0x10000 };
    OsStatus status = km::TlsfHeap::create(range, &heap);
    ASSERT_EQ(OsStatusSuccess, status);

    auto stats0 = heap.stats();

    km::TlsfHeapCommandList list { &heap };

    auto stats1 = heap.stats();

    ASSERT_EQ(stats0.usedMemory, stats1.usedMemory);
    ASSERT_EQ(stats0.freeMemory, stats1.freeMemory);
    ASSERT_EQ(stats0.blockCount, stats1.blockCount);
}

TEST_F(TlsfCommandListTest, DiscardSplit) {
    km::TlsfHeap heap;
    km::MemoryRange range { 0x1000, 0x10000 };
    OsStatus status = km::TlsfHeap::create(range, &heap);
    ASSERT_EQ(OsStatusSuccess, status);

    km::TlsfAllocation allocation = heap.malloc(0x1000);
    km::TlsfAllocation lo, hi;
    auto base = allocation.address();

    auto stats0 = heap.stats();

    // transaction is discarded if not comitted
    {
        km::TlsfHeapCommandList list { &heap };

        OsStatus status = list.split(allocation, base + 0x100, &lo, &hi);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    auto stats1 = heap.stats();

    ASSERT_EQ(stats0.usedMemory, stats1.usedMemory);
    ASSERT_EQ(stats0.freeMemory, stats1.freeMemory);
    ASSERT_EQ(stats0.blockCount, stats1.blockCount);
}

TEST_F(TlsfCommandListTest, CommitSplit) {
    km::TlsfHeap heap;
    km::MemoryRange range { 0x1000, 0x10000 };
    OsStatus status = km::TlsfHeap::create(range, &heap);
    ASSERT_EQ(OsStatusSuccess, status);

    km::TlsfAllocation allocation = heap.malloc(0x1000);
    ASSERT_FALSE(allocation.isNull());

    km::TlsfAllocation lo, hi;
    auto base = allocation.address();

    auto stats0 = heap.stats();

    {
        km::TlsfHeapCommandList list { &heap };

        OsStatus status = list.split(allocation, base + 0x100, &lo, &hi);
        ASSERT_EQ(OsStatusSuccess, status);

        list.commit();
    }

    auto stats1 = heap.stats();

    ASSERT_EQ(stats0.usedMemory, stats1.usedMemory);
    ASSERT_EQ(stats0.freeMemory, stats1.freeMemory);
    ASSERT_LT(stats0.blockCount, stats1.blockCount);

    ASSERT_EQ(lo.address(), base);
    ASSERT_EQ(hi.address(), base + 0x100);
}

TEST_F(TlsfCommandListTest, CommitSplitVector) {
    km::TlsfHeap heap;
    km::MemoryRange range { 0x1000, 0x10000 };
    OsStatus status = km::TlsfHeap::create(range, &heap);
    ASSERT_EQ(OsStatusSuccess, status);

    km::TlsfAllocation allocation = heap.malloc(0x1000);
    ASSERT_FALSE(allocation.isNull());
    auto base = allocation.address();

    std::array<km::TlsfAllocation, 5> results;
    std::array<km::PhysicalAddress, 4> points = {
        base + 0x100,
        base + 0x200,
        base + 0x300,
        base + 0x400,
    };

    auto stats0 = heap.stats();

    {
        km::TlsfHeapCommandList list { &heap };

        OsStatus status = list.splitv(allocation, points, results);
        ASSERT_EQ(OsStatusSuccess, status);

        list.commit();
    }

    auto stats1 = heap.stats();

    ASSERT_EQ(stats0.usedMemory, stats1.usedMemory);
    ASSERT_EQ(stats0.freeMemory, stats1.freeMemory);
    ASSERT_LT(stats0.blockCount, stats1.blockCount);

    ASSERT_EQ(results[0].address(), base);
    ASSERT_EQ(results[1].address(), base + 0x100);
    ASSERT_EQ(results[2].address(), base + 0x200);
    ASSERT_EQ(results[3].address(), base + 0x300);
    ASSERT_EQ(results[4].address(), base + 0x400);
}
