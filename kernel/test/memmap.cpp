#include <gtest/gtest.h>

#include "std/string_view.hpp"

#include "memory/layout.hpp"

void KmDebugWrite(stdx::StringView) { }

void KmBugCheck(stdx::StringView, stdx::StringView, unsigned) {
    abort();
    // GTEST_FAIL_AT(std::string(file.begin(), file.end()).c_str(), line)
    //     << "Bug check triggered: " << std::string_view(message)
    //     << " at " << std::string_view(file) << ":" << line;
}

#if 0

TEST(MemoryMapTest, Basic) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[] = { &e0 };
    limine_memmap_response resp = { 0, std::size(entries), entries };

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, NullEntry) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[] = { &e0, nullptr };
    limine_memmap_response resp = { 0, std::size(entries), entries };

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, ManyNullEntries) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[256] = { &e0 };
    limine_memmap_response resp = { 0, std::size(entries), entries };

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, Overflow) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry e1 = { UINT64_MAX - 0x1000ull, 0x2000000ull, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[] = { &e0, &e1 };
    limine_memmap_response resp = { 0, std::size(entries), entries };

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, TooManyUsableRanges) {
    limine_memmap_entry data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = { uint64_t(0x3000 + i * 0x1000), 0x1000, uint64_t(i % 2 == 0 ? LIMINE_MEMMAP_USABLE : LIMINE_MEMMAP_RESERVED) };
    }

    limine_memmap_entry *entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i] = &data[i];
    }

    limine_memmap_response resp = { 0, std::size(entries), entries };

    [[maybe_unused]]
    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, ReclaimBootMemory) {
    limine_memmap_entry data[24];
    for (size_t i = 0; i < std::size(data); i++) {
        data[i] = { uint64_t(0x3000 + (i * 0x4000)), 0x1000, uint64_t(i % 2 == 0 ? LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE : LIMINE_MEMMAP_USABLE) };
    }

    limine_memmap_entry *entries[std::size(data)];
    for (size_t i = 0; i < std::size(data); i++) {
        entries[i] = &data[i];
    }

    limine_memmap_response resp = { 0, std::size(entries), entries };

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(resp);

    layout.reclaimBootMemory();

    ASSERT_EQ(layout.available.count(), std::size(data)) << "All reclaimable memory ranges reclaimed";
}

TEST(MemoryMapTest, MergeAdjacentDuringReclaim) {
    limine_memmap_entry data[24];
    for (size_t i = 0; i < std::size(data); i++) {
        data[i] = { uint64_t(0x3000 + (i * 0x1000)), 0x1000, uint64_t(i % 2 == 0 ? LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE : LIMINE_MEMMAP_USABLE) };
    }

    limine_memmap_entry *entries[std::size(data)];
    for (size_t i = 0; i < std::size(data); i++) {
        entries[i] = &data[i];
    }

    limine_memmap_response resp = { 0, std::size(entries), entries };

    km::SystemMemoryLayout layout = km::SystemMemoryLayout::from(resp);

    layout.reclaimBootMemory();

    ASSERT_EQ(layout.available.count(), 1) << "All reclaimable memory ranges merged";
    ASSERT_TRUE(layout.reclaimable.isEmpty()) << "All reclaimable memory ranges reclaimed";

    ASSERT_EQ(layout.available[0].front.address, 0x3000) << "Merged range starts at 0x3000";
    ASSERT_EQ(layout.available[0].back.address, 0x3000 + 0x1000 * std::size(data)) << "Merged range ends at 0x3000 + 0x1000 * " << std::size(data);
}
#endif
