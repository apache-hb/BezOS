#include <gtest/gtest.h>
#include <random>

#include "memory/page_allocator.hpp"

TEST(PageAllocatorTest, LowMemorySplit) {
    KernelMemoryMap data;
    size_t size = sm::gigabytes(4).bytes();
    data.add(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x0000uz, size } });

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data);

    km::PhysicalAddress bitmap = sm::megabytes(2).bytes();

    km::detail::RegionAllocators allocators;
    km::detail::LowMemoryAllocators lowMemory;

    km::detail::BuildMemoryRanges(allocators, lowMemory, &layout, bitmap, 0);

    ASSERT_EQ(lowMemory.count(), 1);
    ASSERT_EQ(allocators.count(), 1);

    km::RegionBitmapAllocator& low = lowMemory[0];
    km::RegionBitmapAllocator& high = allocators[0];

    km::MemoryRange low1M = { 0x0000uz, sm::megabytes(1).bytes() };
    ASSERT_EQ(low.range(), low1M);

    km::MemoryRange highMemory = { sm::megabytes(1).bytes(), size };
    ASSERT_EQ(high.range(), highMemory);
}

TEST(PageAllocatorTest, Allocate) {
    KernelMemoryMap data;
    for (int i = 0; i < data.capacity(); i++) {
        size_t size = sm::megabytes(4).bytes();
        void *ptr = _mm_malloc(size, x64::kPageSize);
        km::MemoryRange range = { (uintptr_t)ptr, (uintptr_t)ptr + size };
        data.add({ MemoryMapEntryType::eUsable, range});
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data);

    km::PageAllocator allocator(&layout, 0);

    std::mt19937 rng(0x1234);
    std::uniform_int_distribution<int> dist(0, 4);

    std::vector<std::pair<km::PhysicalAddress, int>> allocated;

    for (size_t i = 0; i < 10000; i++) {
        switch (dist(rng)) {
        case 1: {
            if (!allocated.empty()) {
                std::uniform_int_distribution<size_t> indexDist(0, allocated.size() - 1);
                size_t index = indexDist(rng);
                auto [ptr, pages] = allocated.at(index);
                allocated.erase(allocated.begin() + index);
                allocator.release({ ptr, ptr + (pages * x64::kPageSize) });
                break;
            }
            [[fallthrough]];
        }
        case 0: case 2: case 3: {
            int pages = dist(rng);
            km::PhysicalAddress ptr = allocator.alloc4k(pages);
            if (ptr != nullptr) {
                allocated.push_back({ ptr, pages });
            }
            break;
        }
        }
    }

    for (MemoryMapEntry entry : data) {
        _mm_free((void*)entry.range.front.address);
    }
}
