#include <gtest/gtest.h>

#include "memory/allocator.hpp"

#include "allocator/tlsf.hpp"

static constexpr size_t kSize = 0x10000;

static auto GetAllocatorMemory() {
    auto deleter = [](uint8_t *ptr) {
        :: operator delete[] (ptr, std::align_val_t(16));
    };

    return std::unique_ptr<uint8_t[], decltype(deleter)>{
        new (std::align_val_t(16)) uint8_t[kSize],
        deleter
    };
}

using TestMemory = decltype(GetAllocatorMemory());

class PageTableTest : public testing::Test {
public:
    void SetUp() override {
        memory = GetAllocatorMemory();
        allocator = mem::TlsfAllocator { memory.get(), kSize };
    }

    TestMemory memory;
    mem::TlsfAllocator allocator;
    km::PageBuilder pm { 48, 48, 0, km::PageMemoryTypeLayout { 2, 3, 4, 1, 5, 0 } };
};

TEST_F(PageTableTest, GetBackingAddress) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map4k(paddr, vaddr, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);
    ASSERT_EQ(paddr.address, addr.address);
}

TEST_F(PageTableTest, MapPage) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map4k(paddr, vaddr, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);
}

TEST_F(PageTableTest, MapPageOffset) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map4k(paddr, vaddr, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 123));

    ASSERT_EQ(paddr.address + 123, addr.address);
}

TEST_F(PageTableTest, MapRange) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 64;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.mapRange(range, vaddr, km::PageFlags::eAll);

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = pt.getBackingAddress((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(paddr.address + (i * x64::kPageSize), addr.address);
    }
}

TEST_F(PageTableTest, MapLargePage) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map2m(paddr, vaddr, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);
}

TEST_F(PageTableTest, MapLargePageOffset) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map2m(paddr, vaddr, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 12345));

    ASSERT_EQ(paddr.address + 12345, addr.address);
}

TEST_F(PageTableTest, MapHugePage) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map1g(paddr, vaddr, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);
}

TEST_F(PageTableTest, MapHugePageOffset) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map1g(paddr, vaddr, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 1234567));

    ASSERT_EQ(paddr.address + 1234567, addr.address);
}

TEST_F(PageTableTest, MapRangeLarge) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 1024;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.mapRange(range, vaddr, km::PageFlags::eAll);

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = pt.getBackingAddress((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(paddr.address + (i * x64::kPageSize), addr.address);
    }
}

TEST_F(PageTableTest, MemoryFlags) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;

    pt.map4k(paddr, vaddr, km::PageFlags::eAll);

    km::PageFlags flags = pt.getMemoryFlags(vaddr);

    ASSERT_EQ(km::PageFlags::eAll, flags);
}

TEST_F(PageTableTest, MemoryFlagsOnRange) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 64;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.mapRange(range, vaddr, km::PageFlags::eAll);

    for (size_t i = 0; i < pages; i++) {
        km::PageFlags flags = pt.getMemoryFlags((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageFlags::eAll, flags);
    }
}

TEST_F(PageTableTest, MemoryFlagsOnLargeRange) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 1024;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.mapRange(range, vaddr, km::PageFlags::eCode);

    for (size_t i = 0; i < pages; i++) {
        km::PageFlags flags = pt.getMemoryFlags((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageFlags::eCode, flags);
    }
}

TEST_F(PageTableTest, MemoryFlagsOnNullMapping) {
    km::PageTableManager pt { &pm, &allocator };

    const void *vaddr = (void*)0xFFFF800000000000;

    km::PageFlags flags = pt.getMemoryFlags(vaddr);

    ASSERT_EQ(km::PageFlags::eNone, flags);
}

TEST_F(PageTableTest, MemoryFlagsKernelMapping) {
    uintptr_t kernel = 0x0000000007CC5000;
    uintptr_t hhdm = 0xFFFF800000000000;

    km::AddressMapping text = {
        .vaddr = (void*)0xFFFFFFFF80001000,
        .paddr = (0xFFFFFFFF80001000 - hhdm + kernel),
        .size = (0xFFFFFFFF8004101E - 0xFFFFFFFF80001000),
    };

    km::AddressMapping rodata = {
        .vaddr = (void*)0xFFFFFFFF80042000,
        .paddr = (0xFFFFFFFF80042000 - hhdm + kernel),
        .size = (0xFFFFFFFF80047B88 - 0xFFFFFFFF80042000),
    };

    km::AddressMapping data = {
        .vaddr = (void*)0xFFFFFFFF80048000,
        .paddr = (0xFFFFFFFF80048000 - hhdm + kernel),
        .size = (0xFFFFFFFF80049E40 - 0xFFFFFFFF80048000),
    };

    km::PageTableManager pt { &pm, &allocator };

    pt.map(text, km::PageFlags::eCode);
    pt.map(rodata, km::PageFlags::eRead);
    pt.map(data, km::PageFlags::eData);

    // ensure that all memory is mapped
    for (uintptr_t i = (uintptr_t)text.vaddr; i < (uintptr_t)text.vaddr + text.size; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eCode, flags);

        km::PhysicalAddress addr = pt.getBackingAddress((void*)i);
        ASSERT_EQ(text.paddr + (i - (uintptr_t)text.vaddr), addr.address);
    }

    for (uintptr_t i = (uintptr_t)rodata.vaddr; i < (uintptr_t)rodata.vaddr + rodata.size; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eRead, flags);

        km::PhysicalAddress addr = pt.getBackingAddress((void*)i);
        ASSERT_EQ(rodata.paddr + (i - (uintptr_t)rodata.vaddr), addr.address);
    }

    for (uintptr_t i = (uintptr_t)data.vaddr; i < (uintptr_t)data.vaddr + data.size; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eData, flags);

        km::PhysicalAddress addr = pt.getBackingAddress((void*)i);
        ASSERT_EQ(data.paddr + (i - (uintptr_t)data.vaddr), addr.address);
    }
}
