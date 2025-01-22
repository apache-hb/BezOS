#include <gtest/gtest.h>
#include <random>

#include "memory/layout.hpp"

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

TEST(MemoryMapTest, Basic) {
    KernelMemoryMap memmap;
    memmap.push_back(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x1000, 0x2000 } });

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap, &gAllocator);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, AllEntries) {
    KernelMemoryMap memmap;
    for (size_t i = 0; i < 32; i++) {
        memmap.push_back(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x1000 + i * 0x1000, 0x2000 + i * 0x1000 } });
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap, &gAllocator);

    ASSERT_TRUE(true) << "Layout created successfully";
    ASSERT_FALSE(layout.available.isEmpty()) << "All usable memory ranges added";
}

TEST(MemoryMapTest, ManyRanges) {
    KernelMemoryMap memmap;
    for (int i = 0; i < 32; i++) {
        MemoryMapEntryType type = i % 2 == 0 ? MemoryMapEntryType::eUsable : MemoryMapEntryType::eReserved;
        memmap.push_back(MemoryMapEntry { type, { 0x1000ull + i * 0x1000, 0x2000ull + i * 0x1000 } });
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap, &gAllocator);

    ASSERT_TRUE(true) << "Layout created successfully";

    ASSERT_EQ(layout.available.count(), 16) << "All usable memory ranges added";
    ASSERT_EQ(layout.reserved.count(), 16) << "All reserved memory ranges added";
    ASSERT_EQ(layout.reclaimable.count(), 0) << "No reclaimable memory ranges added";
}

TEST(MemoryMapTest, ReclaimBootMemory) {
    KernelMemoryMap memmap;
    for (int i = 0; i < 32; i++) {
        MemoryMapEntryType type = i % 2 == 0 ? MemoryMapEntryType::eUsable : MemoryMapEntryType::eBootloaderReclaimable;
        uintptr_t start = (0x8000ull * i);
        memmap.push_back(MemoryMapEntry { type, { start, start + 0x1000 } });
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap, &gAllocator);

    layout.reclaimBootMemory();

    ASSERT_GE(layout.available.count(), 1) << "All reclaimable memory ranges reclaimed";
    ASSERT_TRUE(layout.reclaimable.isEmpty()) << "All reclaimable memory ranges reclaimed";
}

TEST(MemoryMapTest, MergeAdjacentDuringReclaim) {
    KernelMemoryMap data;
    for (size_t i = 0; i < 64; i++) {
        MemoryMapEntryType type = i % 2 == 0 ? MemoryMapEntryType::eUsable : MemoryMapEntryType::eBootloaderReclaimable;
        data.push_back(MemoryMapEntry { type, { 0x1000 + (i * 0x1000), 0x2000 + (i * 0x1000) }});
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data, &gAllocator);

    layout.reclaimBootMemory();

    ASSERT_EQ(layout.available.count(), 1) << "All reclaimable memory ranges merged";
    ASSERT_TRUE(layout.reclaimable.isEmpty()) << "All reclaimable memory ranges reclaimed";

    ASSERT_EQ(layout.available[0].front, 0x1000) << "Merged range starts at 0x1000";
    ASSERT_EQ(layout.available[0].back, 0x1000 + 0x1000 * data.size()) << "Merged range ends at 0x1000 + 0x1000 * " << data.size();
}

TEST(MemoryMapTest, EntryListTooLong) {
    KernelMemoryMap data;
    for (size_t i = 0; i < 1024; i++) {
        data.push_back(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x1000 + (i * 0x1000), 0x2000 + (i * 0x1000) }});
    }

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data, &gAllocator);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, UnsortedMemory) {
    std::mt19937 rng(0x9876);

    std::uniform_int_distribution<uintptr_t> dist(0, sm::terabytes(2).bytes());
    KernelMemoryMap memmap;
    for (size_t i = 0; i < 1024; i++) {
        uintptr_t start = dist(rng);
        uintptr_t end = start + dist(rng);
        memmap.push_back(MemoryMapEntry { MemoryMapEntryType::eUsable, { start, end } });
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap, &gAllocator);

    ASSERT_TRUE(std::is_sorted(layout.available.begin(), layout.available.end(), [](const auto& a, const auto& b) {
        return a.front < b.front;
    })) << "Memory ranges are sorted";

    ASSERT_TRUE(std::is_sorted(layout.reserved.begin(), layout.reserved.end(), [](const auto& a, const auto& b) {
        return a.range.front < b.range.front;
    })) << "Memory ranges are sorted";

    ASSERT_TRUE(std::is_sorted(layout.reclaimable.begin(), layout.reclaimable.end(), [](const auto& a, const auto& b) {
        return a.front < b.front;
    })) << "Memory ranges are sorted";
}

TEST(MemoryMapDetailTest, MergeAdjacent) {
    stdx::Vector<km::MemoryRange> ranges(&gAllocator);

    ranges.add({ 0x1000, 0x2000 });
    ranges.add({ 0x2000, 0x3000 });

    km::detail::MergeMemoryRanges(ranges);

    EXPECT_EQ(ranges.count(), 1) << "Adjacent memory ranges merged";
    ASSERT_EQ(ranges[0].front, 0x1000) << "Merged range starts at 0x1000";
    ASSERT_EQ(ranges[0].back.address, 0x3000) << "Merged range ends at 0x3000";
}
