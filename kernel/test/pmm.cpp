#include <gtest/gtest.h>

#include "memory/allocator.hpp"

TEST(PmmTest, BitmapSize) {
    ASSERT_EQ(km::detail::GetRangeBitmapSize({nullptr, nullptr}), 0);

    // 64 kilobytes = 8 pages
    // 8 pages / 8 bits = 1 byte
    ASSERT_EQ(km::detail::GetRangeBitmapSize({(uintptr_t)0x0, sm::kilobytes(32).asBytes()}), 1);

    ASSERT_EQ(km::detail::GetRangeBitmapSize({(uintptr_t)0x0, sm::kilobytes(7).asBytes()}), 1);

    ASSERT_EQ(km::detail::GetRangeBitmapSize({(uintptr_t)0x0, sm::kilobytes(123).asBytes()}), 4);
}

TEST(PmmTest, PageCount) {
    ASSERT_EQ(km::pages(0), 0);

    ASSERT_EQ(km::pages(1), 1);

    ASSERT_EQ(km::pages(4096), 1);

    ASSERT_EQ(km::pages(4097), 2);

    ASSERT_EQ(km::pages(sm::kilobytes(64).asBytes()), 16);
}

TEST(PmmTest, Allocate) {
    size_t size = sm::kilobytes(128).asBytes();
    size_t pages = km::pages(size);

    std::unique_ptr<uint8_t[]> memory = std::make_unique<uint8_t[]>(size);
    uintptr_t memoryAddress = (uintptr_t)memory.get();
    km::MemoryRange range = {memoryAddress, memoryAddress + size};

    size_t bitmapSize = km::detail::GetRangeBitmapSize(range);
    std::unique_ptr<uint8_t[]> bitmap = std::make_unique<uint8_t[]>(bitmapSize);

    km::RegionBitmapAllocator allocator(range, bitmap.get());

    // should be able to allocate
    km::PhysicalAddress addr = allocator.alloc4k(1);
    ASSERT_NE(addr, nullptr);

    // shouldnt return the same address
    km::PhysicalAddress addr2 = allocator.alloc4k(1);
    ASSERT_NE(addr2, nullptr);
    ASSERT_NE(addr, addr2);

    // shouldnt be able to allocate more than the range
    km::PhysicalAddress addr3 = allocator.alloc4k(999999);
    ASSERT_EQ(addr3, nullptr);

    allocator.release({addr, addr + x64::kPageSize});
    allocator.release({addr2, addr2 + x64::kPageSize});

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = allocator.alloc4k(1);
        EXPECT_NE(addr, nullptr);
    }

    allocator.release(range);

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = allocator.alloc4k(1);
        EXPECT_NE(addr, nullptr);
    }

    addr = allocator.alloc4k(1);
    EXPECT_EQ(addr, nullptr);

    allocator.release(range);
}
