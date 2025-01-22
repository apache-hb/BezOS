#include <gtest/gtest.h>

#include <random>

#include "memory/page_allocator.hpp"

using KernelMemoryMap = std::vector<boot::MemoryRegion>;

struct GlobalAllocator final : public mem::IAllocator {
    void *allocateAligned(size_t size, size_t align) override {
        std::cout << "Allocating " << size << " bytes with alignment " << align << std::endl;
        return _mm_malloc(size, align);
    }

    void deallocate(void *ptr, size_t _) override {
        free(ptr);
    }
};

static GlobalAllocator gAllocator;

template<typename T>
using MmUniquePtr = std::unique_ptr<T, decltype(&_mm_free)>;

template<typename T>
MmUniquePtr<T> MakeMmUnique(size_t size, size_t align) {
    return MmUniquePtr<T>((T*)_mm_malloc(size, align), _mm_free);
}

struct TestData {
    KernelMemoryMap memmap;
    km::SystemMemoryLayout layout;
    km::PageAllocator allocator;

    TestData(KernelMemoryMap memmap, km::SystemMemoryLayout layout, mem::IAllocator *alloc)
        : memmap(memmap)
        , layout(layout)
        , allocator(km::PageAllocator(&layout, alloc))
    { }

    ~TestData() {
        for (boot::MemoryRegion entry : memmap) {
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
        data.push_back({ boot::MemoryRegion::eUsable, range});
    }

    return TestData { data, km::SystemMemoryLayout::from(data, &gAllocator), &gAllocator };
}

TEST(PageAllocatorTest, LowMemorySplit) {
    KernelMemoryMap data;
    size_t size = sm::gigabytes(4).bytes();
    data.push_back(boot::MemoryRegion { boot::MemoryRegion::eUsable, { 0x0000uz, size } });

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data, &gAllocator);

    km::PhysicalAddress bitmap = sm::megabytes(2).bytes();

    stdx::Vector<km::RegionBitmapAllocator> allocators(&gAllocator);
    stdx::Vector<km::RegionBitmapAllocator> lowMemory(&gAllocator);

    km::detail::BuildMemoryRanges(allocators, lowMemory, &layout, (uint8_t*)bitmap.address);

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
    auto memory = MakeMmUnique<uint8_t>(sm::megabytes(128).bytes(), x64::kPageSize);

    KernelMemoryMap data;
    for (size_t i = 0; i < 32; i++) {
        boot::MemoryRegion::Type type = i % 2 == 0 ? boot::MemoryRegion::eUsable : boot::MemoryRegion::eBootloaderReclaimable;
        uintptr_t start = (0x8000ull * i) + (uintptr_t)memory.get();
        data.push_back(boot::MemoryRegion { type, { start, start + 0x1000 } });
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data, &gAllocator);

    km::PageAllocator allocator(&layout, &gAllocator);

    layout.reclaimBootMemory();
    allocator.rebuild();
}

TEST(PageAllocatorTest, Alloc4k) {
    static const size_t size = sm::megabytes(128).bytes();
    auto memory = MakeMmUnique<uint8_t>(size, x64::kPageSize);
    km::MemoryRange range = { (uintptr_t)memory.get(), (uintptr_t)memory.get() + size };
    std::unique_ptr<uint8_t[]> bitmap{new uint8_t[km::detail::GetRangeBitmapSize(range)]()};

    km::RegionBitmapAllocator allocator(range, bitmap.get());

    ASSERT_EQ(allocator.range(), range);

    km::PhysicalAddress addr = allocator.alloc4k(1);
    ASSERT_NE(addr, nullptr);

    km::PhysicalAddress addr2 = allocator.alloc4k(1);
    ASSERT_NE(addr2, addr);
}
