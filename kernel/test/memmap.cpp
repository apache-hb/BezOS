#include <gtest/gtest.h>
#include <random>

#include "std/string_view.hpp"

#include "memory/layout.hpp"

void KmDebugWrite(stdx::StringView) { }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn" // GTEST_FAIL_AT is a macro that doesn't return

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) {
    GTEST_FAIL_AT(std::string(file.begin(), file.end()).c_str(), line)
        << "Bug check triggered: " << std::string_view(message)
        << " at " << std::string_view(file) << ":" << line;
}

#pragma clang diagnostic pop

TEST(MemoryMapTest, Basic) {
    KernelMemoryMap memmap;
    memmap.add(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x1000, 0x2000 } });

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, AllEntries) {
    KernelMemoryMap memmap;
    for (size_t i = 0; i < 32; i++) {
        memmap.add(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x1000 + i * 0x1000, 0x2000 + i * 0x1000 } });
    }

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap);

    ASSERT_TRUE(true) << "Layout created successfully";
    ASSERT_FALSE(layout.available.isEmpty()) << "All usable memory ranges added";
}

TEST(MemoryMapTest, ManyRanges) {
    KernelMemoryMap memmap;
    for (int i = 0; i < 32; i++) {
        MemoryMapEntryType type = i % 2 == 0 ? MemoryMapEntryType::eUsable : MemoryMapEntryType::eReserved;
        memmap.add(MemoryMapEntry { type, { 0x1000ull + i * 0x1000, 0x2000ull + i * 0x1000 } });
    }

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap);

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
        memmap.add(MemoryMapEntry { type, { start, start + 0x1000 } });
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap);

    layout.reclaimBootMemory();

    ASSERT_GE(layout.available.count(), 1) << "All reclaimable memory ranges reclaimed";
    ASSERT_TRUE(layout.reclaimable.isEmpty()) << "All reclaimable memory ranges reclaimed";
}

TEST(MemoryMapTest, MergeAdjacentDuringReclaim) {
    KernelMemoryMap data;
    for (size_t i = 0; i < 64; i++) {
        MemoryMapEntryType type = i % 2 == 0 ? MemoryMapEntryType::eUsable : MemoryMapEntryType::eBootloaderReclaimable;
        data.add(MemoryMapEntry { type, { 0x1000 + (i * 0x1000), 0x2000 + (i * 0x1000) }});
    }

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data);

    layout.reclaimBootMemory();

    ASSERT_EQ(layout.available.count(), 1) << "All reclaimable memory ranges merged";
    ASSERT_TRUE(layout.reclaimable.isEmpty()) << "All reclaimable memory ranges reclaimed";

    ASSERT_EQ(layout.available[0].range.front, 0x1000) << "Merged range starts at 0x1000";
    ASSERT_EQ(layout.available[0].range.back, 0x1000 + 0x1000 * data.count()) << "Merged range ends at 0x1000 + 0x1000 * " << data.count();
}

TEST(MemoryMapTest, EntryListTooLong) {
    KernelMemoryMap data;
    for (size_t i = 0; i < data.capacity(); i++) {
        data.add(MemoryMapEntry { MemoryMapEntryType::eUsable, { 0x1000 + (i * 0x1000), 0x2000 + (i * 0x1000) }});
    }

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(data);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, UnsortedMemory) {
    std::mt19937 rng(0x9876);

    std::uniform_int_distribution<uintptr_t> dist(0, sm::terabytes(2).asBytes());
    KernelMemoryMap memmap;
    for (size_t i = 0; i < memmap.capacity(); i++) {
        uintptr_t start = dist(rng);
        uintptr_t end = start + dist(rng);
        memmap.add(MemoryMapEntry { MemoryMapEntryType::eUsable, { start, end } });
    }

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(memmap);

    ASSERT_TRUE(std::is_sorted(layout.available.begin(), layout.available.end(), [](const auto& a, const auto& b) {
        return a.range.front < b.range.front;
    })) << "Memory ranges are sorted";

    ASSERT_TRUE(std::is_sorted(layout.reserved.begin(), layout.reserved.end(), [](const auto& a, const auto& b) {
        return a.range.front < b.range.front;
    })) << "Memory ranges are sorted";

    ASSERT_TRUE(std::is_sorted(layout.reclaimable.begin(), layout.reclaimable.end(), [](const auto& a, const auto& b) {
        return a.range.front < b.range.front;
    })) << "Memory ranges are sorted";
}
