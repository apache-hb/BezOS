#include <gtest/gtest.h>

#include "memory/heap.hpp"
#include "new_shim.hpp"
#include "util/memory.hpp"

using km::TlsfHeap;
using km::TlsfAllocation;

class TlsfHeapDetailTest : public testing::Test {};

TEST_F(TlsfHeapDetailTest, BitScanLeading) {
    static_assert(km::detail::BitScanLeading(0u) == UINT8_MAX);
    static_assert(km::detail::BitScanLeading(0x1u) == 0);
    static_assert(km::detail::BitScanLeading(0x2u) == 1);

    ASSERT_EQ(km::detail::BitScanLeading(0u), UINT8_MAX);
    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(km::detail::BitScanLeading(1u << i), i);
    }
}

TEST_F(TlsfHeapDetailTest, BitScanTrailing) {
    static_assert(km::detail::BitScanTrailing(0u) == UINT8_MAX);
    static_assert(km::detail::BitScanTrailing(0x1u) == 0);
    static_assert(km::detail::BitScanTrailing(0x2u) == 1);

    ASSERT_EQ(km::detail::BitScanTrailing(0u), UINT8_MAX);
    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(km::detail::BitScanTrailing(1u << i), i);
    }
}

TEST_F(TlsfHeapDetailTest, GetFreeListSize) {
    ASSERT_EQ(km::detail::GetFreeListSize(0, 0), 0);
}

class TlsfHeapTest : public testing::Test {
public:
    void SetUp() override {
        auto *allocator = GetGlobalAllocator();
        allocator->reset();
    }

    void TearDown() override {
    }

    static void EnsureHeapSize(TlsfHeap& heap, size_t size) {
        std::map<km::PhysicalAddress, km::TlsfAllocation> allocations;
        for (size_t i = 0; i < size / 0x100; i++) {
            km::TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
            EXPECT_TRUE(addr.isValid());
            EXPECT_FALSE(allocations.contains(addr.address()));
            allocations.emplace(addr.address(), addr);
        }

        auto stats = heap.stats();
        EXPECT_EQ(stats.freeMemory, 0);
        EXPECT_EQ(stats.usedMemory, size);

        for (const auto& [addr, alloc] : allocations) {
            heap.free(alloc);
        }
    }
};

TEST_F(TlsfHeapTest, Construct) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    heap.validate();

    EnsureHeapSize(heap, 0x2000 - 0x1000);
}

TEST_F(TlsfHeapTest, ConstructMany) {
    TlsfHeap heap;
    std::array<km::MemoryRange, 4> ranges {
        km::MemoryRange{0x1000, 0x2000},
        km::MemoryRange{0x2000, 0x3000},
        km::MemoryRange{0x3000, 0x4000},
        km::MemoryRange{0x4000, 0x5000},
    };
    OsStatus status = TlsfHeap::create(ranges, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    heap.validate();

    EnsureHeapSize(heap, 0x5000 - 0x1000);
}

TEST_F(TlsfHeapTest, ConstructManySingle) {
    TlsfHeap heap;
    std::array<km::MemoryRange, 1> ranges {
        km::MemoryRange{0x1000, 0x2000},
    };
    OsStatus status = TlsfHeap::create(ranges, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    heap.validate();

    EnsureHeapSize(heap, 0x2000 - 0x1000);
}

TEST_F(TlsfHeapTest, ConstructManyHoles) {
    TlsfHeap heap;
    std::array<km::MemoryRange, 3> ranges {
        km::MemoryRange{0x1000, 0x2000},
        km::MemoryRange{0x3000, 0x4000},
        km::MemoryRange{0x5000, 0x6000},
    };
    OsStatus status = TlsfHeap::create(ranges, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    heap.validate();

    EnsureHeapSize(heap, 0x3000);
}

TEST_F(TlsfHeapTest, ConstructOutOfMemory) {
    for (size_t i = 0; i < 5; i++) {
        GetGlobalAllocator()->mFailAfter = i;

        TlsfHeap heap;
        OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
        GetGlobalAllocator()->reset();

        EXPECT_TRUE(status == OsStatusOutOfMemory || status == OsStatusSuccess) << "Failed to allocate memory for heap";
        if (status == OsStatusSuccess) {
            heap.validate();
        }
    }
}

TEST_F(TlsfHeapTest, Alloc) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    heap.validate();
}

TEST_F(TlsfHeapTest, Resize) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    TlsfAllocation result;
    status = heap.resize(addr, 0x200, &result);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(result.isValid());

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x200);
    EXPECT_EQ(stats1.usedMemory, 0x200);

    heap.validate();
}

TEST_F(TlsfHeapTest, Malloc) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.malloc(0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    heap.validate();
}

class TlsfHeapReserveTest : public TlsfHeapTest {
public:
    static constexpr km::MemoryRange kTestRange{0x1000, 0x2000};
    void SetUp() override {
        OsStatus status = TlsfHeap::create(kTestRange, &heap);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    km::TlsfHeap heap;
};

TEST_F(TlsfHeapReserveTest, ReserveLow) {
    TlsfAllocation addr;
    OsStatus status = heap.reserve({ 0x1000, 0x1100 }, &addr);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, kTestRange.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    heap.validate();
}

TEST_F(TlsfHeapReserveTest, ReserveMiddle) {
    TlsfAllocation addr;
    OsStatus status = heap.reserve({ 0x1100, 0x1200 }, &addr);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, kTestRange.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    heap.validate();
}

TEST_F(TlsfHeapReserveTest, ReserveHigh) {
    TlsfAllocation addr;
    OsStatus status = heap.reserve({ 0x1F00, 0x2000 }, &addr);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, kTestRange.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    heap.validate();
}

TEST_F(TlsfHeapReserveTest, ReserveAll) {
    TlsfAllocation addr;
    OsStatus status = heap.reserve(kTestRange, &addr);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, 0);
    EXPECT_EQ(stats.usedMemory, kTestRange.size());

    heap.validate();
}

TEST_F(TlsfHeapReserveTest, ReserveAfterAlloc) {
    TlsfAllocation base = heap.malloc(0x100);
    EXPECT_TRUE(base.isValid());
    TlsfAllocation addr;
    OsStatus status = heap.reserve(base.range().offsetBy(0x100), &addr);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, kTestRange.size() - base.size() - addr.size());
    EXPECT_EQ(stats.usedMemory, base.size() + addr.size());

    heap.validate();
}

TEST_F(TlsfHeapReserveTest, ReleaseReservation) {
    TlsfAllocation addr;
    OsStatus status = heap.reserve(kTestRange.subrange(0x100, 0x100), &addr);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, kTestRange.size() - addr.size());
    EXPECT_EQ(stats.usedMemory, addr.size());

    heap.free(addr);

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, kTestRange.size());
    EXPECT_EQ(stats1.usedMemory, 0);

    heap.validate();
}

TEST_F(TlsfHeapTest, Free) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.malloc(0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    heap.free(addr);

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size());
    EXPECT_EQ(stats1.usedMemory, 0);

    heap.validate();
}

TEST_F(TlsfHeapTest, Split) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    heap.validate();

    auto base = addr.address();
    TlsfAllocation lo, hi;
    status = heap.split(addr, base + 0x50, &lo, &hi);
    ASSERT_EQ(status, OsStatusSuccess);

    EXPECT_EQ(lo.address(), base);
    EXPECT_EQ(hi.address(), base + 0x50);

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats1.usedMemory, 0x100);

    heap.validate();
}

TEST_F(TlsfHeapTest, SplitRelease) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    auto base = addr.address();

    TlsfAllocation lo, hi;
    status = heap.split(addr, base + 0x50, &lo, &hi);
    ASSERT_EQ(status, OsStatusSuccess);

    EXPECT_EQ(lo.address(), base);
    EXPECT_EQ(hi.address(), base + 0x50);

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats1.usedMemory, 0x100);

    heap.free(lo);
    auto stats2 = heap.stats();
    EXPECT_EQ(stats2.freeMemory, range.size() - 0x100 + 0x50);
    EXPECT_EQ(stats2.usedMemory, 0x100 - 0x50);
    EXPECT_EQ(hi.address(), base + 0x50);

    heap.validate();
}

TEST_F(TlsfHeapTest, SplitVector) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x4000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x1000);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x1000);
    EXPECT_EQ(stats.usedMemory, 0x1000);

    std::array<km::PhysicalAddress, 15> points;
    std::array<TlsfAllocation, 16> results;
    for (size_t i = 0; i < points.size(); i++) {
        points[i] = addr.address() + 0x100 + (i * 0x100);
    }

    auto base = addr.address();

    status = heap.splitv(addr, points, results);
    ASSERT_EQ(status, OsStatusSuccess);
    for (size_t i = 0; i < results.size(); i++) {
        EXPECT_EQ(results[i].address().address, base.address + (i * 0x100)) << "i: " << i;
    }

    heap.validate();
}

TEST_F(TlsfHeapTest, SplitVectorOom) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x4000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x2000);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - addr.size());
    EXPECT_EQ(stats.usedMemory, addr.size());

    std::vector<km::PhysicalAddress> points;
    std::vector<TlsfAllocation> results;
    for (size_t i = 0; i < (0x2000 / 0x10) - 1; i++) {
        points.push_back(addr.address() + 0x10 + (i * 0x10));
    }
    results.resize(points.size() + 1);

    GetGlobalAllocator()->mFailAfter = 0;
    status = heap.splitv(addr, points, results);
    GetGlobalAllocator()->mFailAfter = SIZE_MAX;
    ASSERT_EQ(status, OsStatusOutOfMemory);
    heap.validate();

    // should be unchanged after oom
    auto stats0 = heap.stats();
    EXPECT_EQ(stats0.freeMemory, range.size() - addr.size());
    EXPECT_EQ(stats0.usedMemory, addr.size());
}

TEST_F(TlsfHeapTest, ResizeSameSize) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    TlsfAllocation result;
    status = heap.resize(addr, 0x100, &result);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(result.isValid());

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats1.usedMemory, 0x100);

    heap.validate();
}

TEST_F(TlsfHeapTest, ShrinkSameSize) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    TlsfAllocation result;
    status = heap.shrink(addr, 0x100, &result);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(result.isValid());

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats1.usedMemory, 0x100);
}

TEST_F(TlsfHeapTest, GrowSameSize) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    TlsfAllocation result;
    status = heap.grow(addr, 0x100, &result);
    EXPECT_EQ(status, OsStatusSuccess);
    EXPECT_TRUE(result.isValid());

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats1.usedMemory, 0x100);
}

TEST_F(TlsfHeapTest, GrowSmaller) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    TlsfAllocation result;
    status = heap.grow(addr, 0x50, &result);
    EXPECT_EQ(status, OsStatusInvalidInput);
    EXPECT_FALSE(result.isValid());

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats1.usedMemory, 0x100);
}

TEST_F(TlsfHeapTest, ShrinkLarger) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats.usedMemory, 0x100);

    TlsfAllocation result;
    status = heap.shrink(addr, 0x200, &result);
    EXPECT_EQ(status, OsStatusInvalidInput);
    EXPECT_FALSE(result.isValid());

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size() - 0x100);
    EXPECT_EQ(stats1.usedMemory, 0x100);
}

TEST_F(TlsfHeapTest, ShrinkOom) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x800);
    EXPECT_TRUE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size() - 0x800);
    EXPECT_EQ(stats.usedMemory, 0x800);

    for (size_t i = 0; i < 60; i++) {
        GetGlobalAllocator()->mFailAfter = 0;
        size_t size = 0x800 - (i * 0x20);
        TlsfAllocation result;
        status = heap.shrink(addr, size, &result);
        GetGlobalAllocator()->reset();

        if (status == OsStatusOutOfMemory) {
            break;
        } else {
            EXPECT_EQ(status, OsStatusSuccess);

            auto stats1 = heap.stats();
            EXPECT_EQ(stats1.freeMemory, range.size() - size) << "Failed to shrink to size: " << size << " (iteration: " << i << ")";
            EXPECT_EQ(stats1.usedMemory, size) << "Failed to shrink to size: " << size << " (iteration: " << i << ")";
        }
    }
}

TEST_F(TlsfHeapTest, NullAddress) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x10000000);
    EXPECT_TRUE(addr.isNull());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, range.size());
    EXPECT_EQ(stats.usedMemory, 0);
}

TEST_F(TlsfHeapTest, AllocMassive) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, sm::terabytes(2).bytes());
    EXPECT_FALSE(addr.isValid());
}

TEST_F(TlsfHeapTest, AllocMany) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x2000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    for (size_t i = 0; i < (0x1000 / 0x100); i++) {
        TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }

    // memory should be exhausted
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_FALSE(addr.isValid());

    auto stats = heap.stats();
    EXPECT_EQ(stats.freeMemory, 0);
    EXPECT_EQ(stats.usedMemory, range.size());
}

TEST_F(TlsfHeapTest, AllocFree) {
    km::MemoryRange range{0x1000, 0x2000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < (range.size() / 0x200); i++) {
        TlsfAllocation addr = heap.aligned_alloc(0x10, 0x200);
        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)addr.address().address;
        pointers.insert(addr);
    }

    // memory should be exhausted
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_FALSE(addr.isValid());

    auto stats0 = heap.stats();
    EXPECT_EQ(stats0.freeMemory, 0);
    EXPECT_EQ(stats0.usedMemory, range.size());

    // GetGlobalAllocator()->mNoAlloc = true;
    for (auto addr : pointers) {
        heap.free(addr);
    }
    // GetGlobalAllocator()->mNoAlloc = false;
    pointers.clear();

    auto stats1 = heap.stats();
    EXPECT_EQ(stats1.freeMemory, range.size());
    EXPECT_EQ(stats1.usedMemory, 0);

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)addr.address().address;
        pointers.insert(addr);
    }
}

// free random elements
TEST_F(TlsfHeapTest, AllocFreeSlots) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);

    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t align = 1 << alignDistribution(random);
        size_t size = sizeDistribution(random) * align;
        TlsfAllocation addr = heap.aligned_alloc(align, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)addr.address().address;
        pointers.insert(addr);
    }

    TlsfAllocation alloc;
    for (size_t i = 0; i < 32; i++) {
        // select random element
        size_t index = random() % pointers.size();
        auto it = pointers.begin();
        std::advance(it, index);
        alloc = *it;
        EXPECT_TRUE(alloc.isValid());
        EXPECT_TRUE(pointers.contains(alloc)) << "Pointer not found: " << (void*)alloc.address().address;

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(alloc);
        GetGlobalAllocator()->mNoAlloc = false;
        pointers.erase(it);
    }

    GetGlobalAllocator()->mNoAlloc = true;
    for (auto addr : pointers) {
        heap.free(addr);
    }
    GetGlobalAllocator()->mNoAlloc = false;
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)addr.address().address;
        pointers.insert(addr);
    }
}

TEST_F(TlsfHeapTest, AllocReverse) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t align = 1 << alignDistribution(random);
        size_t size = sizeDistribution(random) * align;
        TlsfAllocation addr = heap.aligned_alloc(align, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        pointers.push_back(addr);
    }

    for (auto it = pointers.rbegin(); it != pointers.rend(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }
}

TEST_F(TlsfHeapTest, AllocForward) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t align = 1 << alignDistribution(random);
        size_t size = sizeDistribution(random) * align;
        TlsfAllocation addr = heap.aligned_alloc(align, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        pointers.push_back(addr);
    }

    for (auto it = pointers.begin(); it != pointers.end(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }
}

TEST_F(TlsfHeapTest, AllocSizeVarying) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> sizeDistribution(2, 16);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t size = sizeDistribution(random) * 16 * 0x10;
        TlsfAllocation addr = heap.aligned_alloc(0x10, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        pointers.push_back(addr);
    }

    for (auto it = pointers.begin(); it != pointers.end(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }
}

TEST_F(TlsfHeapTest, AllocLarge) {
    km::MemoryRange range{0x1000, 0x40000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto a1 = heap.aligned_alloc(0x1000, 0x8000);
    ASSERT_TRUE(a1.isValid());

    heap.free(a1);
}

TEST_F(TlsfHeapTest, AllocSeq) {
    km::MemoryRange range{0x1000, 0x40000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto a1 = heap.aligned_alloc(8, 16);
    ASSERT_TRUE(a1.isValid());

    auto a2 = heap.aligned_alloc(4, 12);
    ASSERT_TRUE(a2.isValid());

    auto a3 = heap.aligned_alloc(8, 24);
    ASSERT_TRUE(a3.isValid());

    heap.free(a2);
    heap.free(a3);
    heap.free(a1);

    auto a4 = heap.aligned_alloc(256, 2048 * 4);
    ASSERT_TRUE(a4.isValid());

    heap.free(a4);

    auto a5 = heap.aligned_alloc(8, 2048 * 4);
    ASSERT_TRUE(a5.isValid());
}

TEST_F(TlsfHeapTest, AllocPrevFree) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto a1 = heap.aligned_alloc(0x10, 0x100);
    ASSERT_TRUE(a1.isValid());

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 1; i < 6; i++) {
        auto addr = heap.aligned_alloc((1 << i), i * 0x100);
        ASSERT_TRUE(addr.isValid());
        pointers.push_back(addr);
    }

    for (auto it = pointers.rbegin(); it != pointers.rend(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
    heap.free(a1);

    auto a3 = heap.aligned_alloc((1 << 8), 0x800);
    ASSERT_TRUE(a3.isValid());

    heap.free(a3);
}

// random access allocations
TEST_F(TlsfHeapTest, AllocFreeRandom) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 100);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < 200; i++) {
        if (pointers.size() > 0 && distribution(random) < 50) {
            auto it = pointers.begin();
            std::advance(it, distribution(random) % pointers.size());

            GetGlobalAllocator()->mNoAlloc = true;
            heap.free(*it);
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(it);
        } else {
            TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
            if (addr.isNull()) {
                continue;
            }

            EXPECT_TRUE(addr.isValid());

            ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)addr.address().address;
            pointers.insert(addr);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// random access allocations
TEST_F(TlsfHeapTest, AllocFreeRandomAlign) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 100);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < 200; i++) {
        if (pointers.size() > 0 && distribution(random) < 50) {
            auto it = pointers.begin();
            std::advance(it, distribution(random) % pointers.size());

            GetGlobalAllocator()->mNoAlloc = true;
            heap.free(*it);
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(it);
        } else {
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            TlsfAllocation addr = heap.aligned_alloc(align, size);
            if (addr.isNull()) {
                continue;
            }

            EXPECT_TRUE(addr.isValid());

            ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)addr.address().address;
            pointers.insert(addr);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, AllocChunk) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            auto ptr = heap.aligned_alloc(0x10, 0x100);
            if (!ptr.isValid()) {
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)ptr.address().address;
            pointers.push_back(ptr);
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, AllocChunkRandomSize) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            if (!ptr.isValid()) {
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)ptr.address().address;
            pointers.push_back(ptr);

            EXPECT_TRUE(ptr.address().address % align == 0) << "Pointer not aligned: " << align << " ptr " << (void*)ptr.address().address;
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, AllocChunkRandomSizeOom) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            GetGlobalAllocator()->mFailPercent = 0.5f;
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            GetGlobalAllocator()->mFailPercent = 0.f;

            if (!ptr.isValid()) {
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)ptr.address().address;
            pointers.push_back(ptr);

            EXPECT_TRUE(ptr.address().address % align == 0) << "Pointer not aligned: " << align << " ptr " << (void*)ptr.address().address;
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, OomDoesntLeakBlocks) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            km::TlsfHeapStats before = heap.stats();

            GetGlobalAllocator()->mFailPercent = 0.5f;
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            GetGlobalAllocator()->mFailPercent = 0.f;

            if (!ptr.isValid()) {
                km::TlsfHeapStats after = heap.stats();
                EXPECT_EQ(before.pool.freeSlots, after.pool.freeSlots) << "Free blocks should not change on OOM";
                ASSERT_NE(after.controlMemory(), 0) << "Used memory should not be zero";
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)ptr.address().address;
            pointers.push_back(ptr);
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

TEST_F(TlsfHeapTest, AllocateAt) {
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    TlsfAllocation addr = heap.allocateAt(km::PhysicalAddressEx{0x2000}, 0x100);
    EXPECT_TRUE(addr.isValid());
    EXPECT_EQ(addr.address().address, 0x2000);

    auto stats = heap.stats();
    EXPECT_EQ(stats.usedMemory, 0x100);
    EXPECT_EQ(stats.freeMemory, range.size() - 0x100);
}

#if 0
TEST_F(TlsfHeapTest, CreateMany) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    km::MemoryRange range2{0x20000, 0x3000000};
    km::MemoryRange range3{0x80000000, 0x90000000};
    std::array ranges = std::to_array({range, range2, range3});

    OsStatus status = TlsfHeap::create(ranges, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats0 = heap.stats();
    EXPECT_EQ(stats0.freeMemory, std::accumulate(ranges.begin(), ranges.end(), 0zu, [](size_t sum, const km::MemoryRange& range) {
        return sum + range.size();
    }));
    EXPECT_EQ(stats0.usedMemory, 0);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 200; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            km::TlsfHeapStats before = heap.stats();

            // GetGlobalAllocator()->mFailPercent = 0.5f;
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            // GetGlobalAllocator()->mFailPercent = 0.f;

            if (!ptr.isValid()) {
                km::TlsfHeapStats after = heap.stats();
                EXPECT_EQ(before.pool.freeSlots, after.pool.freeSlots) << "Free blocks should not change on OOM";
                ASSERT_NE(after.controlMemory(), 0) << "Used memory should not be zero";
                break;
            }

            ASSERT_TRUE(std::ranges::find_if(ranges, [&](const km::MemoryRange& range) {
                return range.contains(ptr.address());
            }) != ranges.end()) << "Pointer not in range: " << (void*)ptr.address().address;

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)ptr.address().address;
            pointers.push_back(ptr);
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    auto stats1 = heap.stats();
    EXPECT_NE(stats1.freeMemory, stats0.freeMemory);
    EXPECT_NE(stats1.usedMemory, 0);

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    auto stats2 = heap.stats();
    EXPECT_EQ(stats2.freeMemory, stats0.freeMemory);
    EXPECT_EQ(stats2.usedMemory, 0);
}

TEST_F(TlsfHeapTest, CreateManyOom) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);

    std::vector<km::MemoryRange> ranges;
    for (size_t i = 0; i < 100; i++) {
        km::MemoryRange range { 0x10000 + (i * 0x1000), 0x11000 + (i * 0x1000) };
        ranges.push_back(range);
    }

    for (size_t i = 0; i < 200; i++) {
        GetGlobalAllocator()->mFailAfter = i;

        TlsfHeap heap;
        OsStatus status = TlsfHeap::create(ranges, &heap);
        GetGlobalAllocator()->reset();

        EXPECT_TRUE(status == OsStatusOutOfMemory || status == OsStatusSuccess) << "Failed to allocate memory for heap";
        if (status == OsStatusSuccess) {
            heap.validate();
        }
    }
}

TEST_F(TlsfHeapTest, CreateManyEmpty) {
    km::MemoryRange empty{ 0x1000, 0x1000 };
    std::array ranges = std::to_array({ empty });

    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(ranges, &heap);
    EXPECT_EQ(status, OsStatusInvalidInput) << "Empty ranges should not be valid";
}

TEST_F(TlsfHeapTest, CreateManyEmptyList) {
    std::vector<km::MemoryRange> empty;

    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(empty, &heap);
    EXPECT_EQ(status, OsStatusInvalidInput) << "Empty ranges should not be valid";
}

TEST_F(TlsfHeapTest, CreateManyEmptyElement) {
    std::vector<km::MemoryRange> list = {
        {0x1000, 0x2000},
        {0x2000, 0x2000},
    };

    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(list, &heap);
    EXPECT_EQ(status, OsStatusInvalidInput) << "Empty ranges should not be valid";
}
#endif

TEST_F(TlsfHeapTest, ResizeAllocSingle) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};

    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats0 = heap.stats();
    EXPECT_EQ(stats0.freeMemory, range.size());
    EXPECT_EQ(stats0.usedMemory, 0);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            km::TlsfHeapStats before = heap.stats();

            GetGlobalAllocator()->mFailPercent = 0.5f;
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            GetGlobalAllocator()->mFailPercent = 0.f;

            if (!ptr.isValid()) {
                km::TlsfHeapStats after = heap.stats();
                EXPECT_EQ(before.pool.freeSlots, after.pool.freeSlots) << "Free blocks should not change on OOM";
                ASSERT_NE(after.controlMemory(), 0) << "Used memory should not be zero";
                break;
            }

            auto base = ptr.address();

            if (distribution(random) % 10 == 0) {
                size_t newSize = (size / 2);
                TlsfAllocation lo, hi;
                if (heap.split(ptr, base + newSize, &lo, &hi) == OsStatusSuccess) {
                    EXPECT_TRUE(lo.isValid());
                    EXPECT_TRUE(hi.isValid());
                    EXPECT_EQ(lo.address().address, base.address);
                    EXPECT_EQ(hi.address().address, base.address + newSize);
                    heap.free(lo);
                    heap.free(hi);
                } else {
                    EXPECT_FALSE(lo.isValid());
                    EXPECT_FALSE(hi.isValid());
                    EXPECT_TRUE(ptr.isValid());
                }
                break;
            }

            ASSERT_TRUE(range.contains(ptr.address())) << "Pointer not in range: " << (void*)ptr.address().address << " (iteration: " << i << ")";

            EXPECT_TRUE(ptr.isValid());
            EXPECT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)ptr.address().address << " (iteration: " << i << ")";
            pointers.push_back(ptr);
        }

        if (distribution(random) < 24) {
            size_t index = distribution(random) % pointers.size();
            GetGlobalAllocator()->mFailPercent = 0.3f;
            switch (distribution(random) % 3) {
                case 0: {
                    TlsfAllocation newPtr;
                    if (heap.resize(pointers[index], sizeDistribution(random) * 0x10, &newPtr) == OsStatusSuccess) {
                        EXPECT_TRUE(newPtr.isValid());
                        pointers.erase(std::remove(pointers.begin(), pointers.end(), pointers[index]), pointers.end());
                        EXPECT_TRUE(std::ranges::find(pointers, newPtr) == pointers.end()) << "Duplicate pointer: " << (void*)newPtr.address().address;
                        pointers.push_back(newPtr);
                    } else {
                        EXPECT_FALSE(newPtr.isValid()) << "Failed to resize pointer: " << (void*)pointers[index].address().address;
                    }
                    break;
                }
                case 1: {
                    GetGlobalAllocator()->mNoAlloc = true;
                    TlsfAllocation newPtr;
                    if (heap.grow(pointers[index], sizeDistribution(random) * 0x10, &newPtr) == OsStatusSuccess) {
                        GetGlobalAllocator()->mNoAlloc = false;
                        EXPECT_TRUE(newPtr.isValid());

                        pointers.erase(std::remove(pointers.begin(), pointers.end(), pointers[index]), pointers.end());
                        EXPECT_TRUE(std::ranges::find(pointers, newPtr) == pointers.end()) << "Duplicate pointer: " << (void*)newPtr.address().address;
                        pointers.push_back(newPtr);
                    } else {
                        EXPECT_FALSE(newPtr.isValid());
                    }
                    GetGlobalAllocator()->mNoAlloc = false;
                    break;
                }
                #if 0
            case 2: {
                (void)heap.shrink(pointers[index], sizeDistribution(random) * 0x10);
                break;
            }
#endif
            }
            GetGlobalAllocator()->mFailPercent = 0.f;
        }

        if (distribution(random) < 24) {
            // GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            // GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    auto stats1 = heap.stats();
    EXPECT_NE(stats1.freeMemory, stats0.freeMemory);
    EXPECT_NE(stats1.usedMemory, 0);

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    auto stats2 = heap.stats();
    EXPECT_EQ(stats2.freeMemory, stats0.freeMemory);
    EXPECT_EQ(stats2.usedMemory, 0);
}

#if 0
TEST_F(TlsfHeapTest, ResizeAlloc) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    km::MemoryRange range2{0x20000, 0x3000000};
    km::MemoryRange range3{0x80000000, 0x90000000};
    std::array ranges = std::to_array({range, range2, range3});

    OsStatus status = TlsfHeap::create(ranges, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto stats0 = heap.stats();
    EXPECT_EQ(stats0.freeMemory, std::accumulate(ranges.begin(), ranges.end(), 0zu, [](size_t sum, const km::MemoryRange& range) {
        return sum + range.size();
    }));
    EXPECT_EQ(stats0.usedMemory, 0);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            km::TlsfHeapStats before = heap.stats();

            GetGlobalAllocator()->mFailPercent = 0.5f;
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            GetGlobalAllocator()->mFailPercent = 0.f;

            if (!ptr.isValid()) {
                km::TlsfHeapStats after = heap.stats();
                EXPECT_EQ(before.pool.freeSlots, after.pool.freeSlots) << "Free blocks should not change on OOM";
                ASSERT_NE(after.controlMemory(), 0) << "Used memory should not be zero";
                break;
            }

            auto base = ptr.address();

            if (distribution(random) % 10 == 0) {
                size_t newSize = (size / 2);
                TlsfAllocation lo, hi;
                if (heap.split(ptr, base + newSize, &lo, &hi) == OsStatusSuccess) {
                    EXPECT_TRUE(lo.isValid());
                    EXPECT_TRUE(hi.isValid());
                    EXPECT_EQ(lo.address().address, base.address);
                    EXPECT_EQ(hi.address().address, base.address + newSize);
                    heap.free(lo);
                    heap.free(hi);
                } else {
                    EXPECT_FALSE(lo.isValid());
                    EXPECT_FALSE(hi.isValid());
                    EXPECT_TRUE(ptr.isValid());
                }
                break;
            }

            ASSERT_TRUE(std::ranges::find_if(ranges, [&](const km::MemoryRange& range) {
                return range.contains(ptr.address());
            }) != ranges.end()) << "Pointer not in range: " << (void*)ptr.address().address << " (iteration: " << i << ")";

            EXPECT_TRUE(ptr.isValid());
            EXPECT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)ptr.address().address << " (iteration: " << i << ")";
            pointers.push_back(ptr);
        }

        heap.validate();


        if (distribution(random) < 24) {
            size_t index = distribution(random) % pointers.size();
            GetGlobalAllocator()->mFailPercent = 0.0f;
            switch (distribution(random) % 3) {
                #if 0
            case 0: {
                (void)heap.resize(pointers[index], sizeDistribution(random) * 0x10);
                break;
            }
            #endif
            case 1: {
                // GetGlobalAllocator()->mNoAlloc = true;
                (void)heap.grow(pointers[index], sizeDistribution(random) * 0x10);
                // GetGlobalAllocator()->mNoAlloc = false;
                break;
            }
            #if 0
            case 2: {
                (void)heap.shrink(pointers[index], sizeDistribution(random) * 0x10);
                break;
            }
            #endif
            }
            GetGlobalAllocator()->mFailPercent = 0.f;
        }


        if (distribution(random) < 24) {
            // GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            // GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    auto stats1 = heap.stats();
    EXPECT_NE(stats1.freeMemory, stats0.freeMemory);
    EXPECT_NE(stats1.usedMemory, 0);

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    auto stats2 = heap.stats();
    EXPECT_EQ(stats2.freeMemory, stats0.freeMemory);
    EXPECT_EQ(stats2.usedMemory, 0);
}
#endif
