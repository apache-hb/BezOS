#include <gtest/gtest.h>

#include "memory/detail/heap.hpp"
#include "test/new_shim.hpp"

using km::TlsfHeap;
using km::TlsfAllocation;

class TlsfHeapDetailTest : public testing::Test {};

TEST_F(TlsfHeapDetailTest, BitScanLeading) {
    static_assert(km::detail::BitScanLeading(0) == UINT8_MAX);
    static_assert(km::detail::BitScanLeading(0x1) == 0);
    static_assert(km::detail::BitScanLeading(0x2) == 1);

    ASSERT_EQ(km::detail::BitScanLeading(0), UINT8_MAX);
    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(km::detail::BitScanLeading(1u << i), i);
    }
}

TEST_F(TlsfHeapDetailTest, BitScanTrailing) {
    static_assert(km::detail::BitScanTrailing(0) == UINT8_MAX);
    static_assert(km::detail::BitScanTrailing(0x1) == 0);
    static_assert(km::detail::BitScanTrailing(0x2) == 1);

    ASSERT_EQ(km::detail::BitScanTrailing(0), UINT8_MAX);
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
};

TEST_F(TlsfHeapTest, Construct) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
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
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());
}

TEST_F(TlsfHeapTest, AllocMany) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    for (size_t i = 0; i < (0x1000 / 0x100); i++) {
        TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }

    // memory should be exhausted
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_FALSE(addr.isValid());
}

TEST_F(TlsfHeapTest, AllocFree) {
    km::MemoryRange range{0x1000, 0x2000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
        pointers.insert(addr);
    }

    // memory should be exhausted
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_FALSE(addr.isValid());

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

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
        pointers.insert(addr);
    }
}
