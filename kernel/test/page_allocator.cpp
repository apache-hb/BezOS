#include <gtest/gtest.h>

#include <random>

#include "memory/page_allocator.hpp"

using KernelMemoryMap = std::vector<boot::MemoryRegion>;

struct GlobalAllocator final : public mem::IAllocator {
    void *allocateAligned(size_t size, size_t align) override {
        return _mm_malloc(size, align);
    }

    void deallocate(void *ptr, size_t _) override {
        free(ptr);
    }
};

static GlobalAllocator gAllocator;

struct TestData {
    KernelMemoryMap memmap;
    km::SystemMemoryLayout layout;
    km::PageAllocator allocator;

    ~TestData() {
        for (MemoryMapEntry entry : memmap) {
            _mm_free((void*)entry.range.front.address);
        }
    }
};

static TestData GetTestAllocator() {
    KernelMemoryMap data;
    for (unsigned i = 0; i < 64; i++) {
        size_t size = sm::megabytes(4).bytes();
        void *ptr = _mm_malloc(size, x64::kPageSize);
        km::MemoryRange range = { (uintptr_t)ptr, (uintptr_t)ptr + size };
        data.push_back({ MemoryMapEntryType::eUsable, range});
    }

    TestData test = { data, km::SystemMemoryLayout::from(data, &gAllocator), km::PageAllocator(&test.layout, 0, &gAllocator) };

    return test;
}

TEST(PageAllocatorTest, LowMemorySplit) {
    KernelMemoryMap data;
    size_t size = sm::gigabytes(4).bytes();
    data.push_back(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x0000uz, size } });

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data, &gAllocator);

    km::PhysicalAddress bitmap = sm::megabytes(2).bytes();

    stdx::Vector<km::RegionBitmapAllocator> allocators(&gAllocator);
    stdx::Vector<km::RegionBitmapAllocator> lowMemory(&gAllocator);

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
    auto [_, _, allocator] = GetTestAllocator();

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

    for (auto [ptr, pages] : allocated) {
        allocator.release({ ptr, ptr + (pages * x64::kPageSize) });
    }
}

TEST(PageAllocatorTest, RebuildAfterReclaim) {
    void *memory = _mm_malloc(sm::megabytes(128).bytes(), x64::kPageSize);

    KernelMemoryMap data;
    for (size_t i = 0; i < 32; i++) {
        MemoryMapEntryType type = i % 2 == 0 ? MemoryMapEntryType::eUsable : MemoryMapEntryType::eBootloaderReclaimable;
        uintptr_t start = (0x8000ull * i) + (uintptr_t)memory;
        data.push_back(MemoryMapEntry { type, { start, start + 0x1000 } });
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data, &gAllocator);

    km::PageAllocator allocator(&layout, 0, &gAllocator);

    layout.reclaimBootMemory();
    allocator.rebuild();

    _mm_free(memory);
}
