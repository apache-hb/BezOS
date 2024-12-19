#include <gtest/gtest.h>

#include "memory/physical.hpp"

#include <limine.h>

void KmDebugWrite(stdx::StringView) { }

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) {
    ASSERT_TRUE(false) << "Bug check triggered: " << std::string_view(message) << " at " << std::string_view(file) << ":" << line;
}

TEST(MemoryMapTest, Basic) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[] = { &e0 };
    limine_memmap_response resp = { 0, std::size(entries), entries };
    km::PhysicalMemoryLayout layout(&resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, NullEntry) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[] = { &e0, nullptr };
    limine_memmap_response resp = { 0, std::size(entries), entries };
    km::PhysicalMemoryLayout layout(&resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, ManyNullEntries) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[256] = { &e0 };
    limine_memmap_response resp = { 0, std::size(entries), entries };

    km::PhysicalMemoryLayout layout(&resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}

TEST(MemoryMapTest, Overflow) {
    limine_memmap_entry e0 = { 0x1000, 0x1000, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry e1 = { UINT64_MAX - 0x1000ull, 0x2000000ull, LIMINE_MEMMAP_USABLE };
    limine_memmap_entry *entries[] = { &e0, &e1 };
    limine_memmap_response resp = { 0, std::size(entries), entries };
    km::PhysicalMemoryLayout layout(&resp);

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
    km::PhysicalMemoryLayout layout(&resp);

    ASSERT_TRUE(true) << "Layout created successfully";
}
