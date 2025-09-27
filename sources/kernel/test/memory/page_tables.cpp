#include <gtest/gtest.h>
#include <memory_resource>

#include "memory/page_tables.hpp"
#include "arch/paging.hpp"
#include "setup.hpp"

#include "common/compiler/compiler.hpp"

using namespace km;

static constexpr size_t kSize = 0x10000;

auto GetAllocatorMemory(size_t size = kSize) {
    auto deleter = [](uint8_t *ptr) {
        :: operator delete[] (ptr, std::align_val_t(x64::kPageSize));
    };

    return std::unique_ptr<uint8_t[], decltype(deleter)>{
        new (std::align_val_t(x64::kPageSize)) uint8_t[size],
        deleter
    };
}

using TestMemory = decltype(GetAllocatorMemory());

struct BufferAllocator final : public mem::IAllocator {
    std::pmr::monotonic_buffer_resource mResource;

public:
    BufferAllocator(uint8_t *memory, size_t size)
        : mResource(memory, size)
    { }

    void *allocateAligned(size_t size, size_t align) override {
        return mResource.allocate(size, align);
    }

    void deallocate(void *ptr, size_t size) noexcept [[clang::nonallocating]] override {
        CLANG_DIAGNOSTIC_PUSH();
        CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");
        mResource.deallocate(ptr, size);
        CLANG_DIAGNOSTIC_POP();
    }
};

class PageTableTest : public testing::Test {
public:
    void SetUp() override {
    }

    TestMemory memory;
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };

    km::PageTables ptes(km::PageFlags flags = km::PageFlags::eAll, size_t size = (64 * x64::kPageSize)) {
        memory = GetAllocatorMemory(size);
        km::AddressMapping mapping { memory.get(), (uintptr_t)memory.get(), size };
        km::PageTables result;
        if (OsStatus status = km::PageTables::create(&pm, mapping, flags, &result)) {
            throw std::runtime_error(std::format("Failed to create page tables: {}", status));
        }
        return result;
    }
};

static AddressMapping MapArea(
    km::PageTables& ptes,
    uintptr_t vaddr, uintptr_t paddr, size_t size,
    PageFlags flags = PageFlags::eAll,
    MemoryType type = MemoryType::eWriteBack
) {
    const void *v = (void*)vaddr;
    km::PhysicalAddress p = paddr;
    km::AddressMapping mapping { v, p, size };

    OsStatus status = ptes.map(mapping, flags, type);
    if (status != OsStatusSuccess) {
        throw std::runtime_error("Failed to map memory");
    }

    return mapping;
}

static void IsMapped(PageTables& ptes, AddressMapping mapping, PageFlags flags) {
    for (uintptr_t i = (uintptr_t)mapping.vaddr; i < (uintptr_t)mapping.vaddr + mapping.size; i += x64::kPageSize) {
        uintptr_t offset = i - (uintptr_t)mapping.vaddr;

        km::PageFlags f = ptes.getMemoryFlags((void*)i);
        ASSERT_EQ(flags, f)
            << "Address: " << (void*)i
            << " Expected: " << (int)flags
            << " Got: " << (int)f;

        km::PhysicalAddress addr = ptes.getBackingAddress((void*)i);
        PhysicalAddress expected = mapping.paddr + offset;
        ASSERT_EQ(expected.address, addr.address)
            << "Address: " << (void*)i
            << " Expected: " << (void*)(expected.address)
            << " Got: " << (void*)(addr.address);
    }
}

static void IsNotMapped(PageTables& ptes, VirtualRange range) {
    for (uintptr_t i = (uintptr_t)range.front; i < (uintptr_t)range.back; i += x64::kPageSize) {
        km::PageFlags f = ptes.getMemoryFlags((void*)i);
        EXPECT_EQ(PageFlags::eNone, f)
            << "Address: " << (void*)i;

        PageSize size = ptes.getPageSize((void*)i);
        EXPECT_EQ(PageSize::eNone, size)
            << "Address: " << (void*)i;

        km::PhysicalAddress addr = ptes.getBackingAddress((void*)i);
        ASSERT_EQ(KM_INVALID_MEMORY, addr)
            << "Address: " << (void*)i
            << " Got: " << (void*)(addr.address);
    }
}

TEST_F(PageTableTest, Unmap4kPage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kPageSize * 32);
    auto [vaddr, paddr, size] = mapping;

    IsMapped(pt, mapping, km::PageFlags::eAll);

    ASSERT_EQ(km::PageSize::eRegular, pt.getPageSize(vaddr));

    // unmap a few 4k pages
    km::VirtualRange range = km::VirtualRange::of((void*)((uintptr_t)vaddr + x64::kPageSize * 4), x64::kPageSize * 4);
    OsStatus status = pt.unmap(range);
    ASSERT_EQ(OsStatusSuccess, status);

    // the first and third 2m pages should still be mapped
    IsMapped(pt, mapping.first(x64::kPageSize * 4), km::PageFlags::eAll);

    IsNotMapped(pt, range);

    // the remaining pages should still be mapped as 4k pages
    IsMapped(pt, mapping.last(mapping.size - ((x64::kPageSize * 4) + range.size())), km::PageFlags::eAll);
}

TEST_F(PageTableTest, Unmap2mPage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 4);
    auto [vaddr, paddr, size] = mapping;

    IsMapped(pt, mapping, km::PageFlags::eAll);

    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // unmap the second 2m page
    km::VirtualRange range = km::VirtualRange::of((void*)((uintptr_t)vaddr + x64::kLargePageSize), x64::kLargePageSize);
    OsStatus status = pt.unmap(range);
    ASSERT_EQ(OsStatusSuccess, status);

    // the first and third 2m pages should still be mapped
    IsMapped(pt, mapping.first(x64::kLargePageSize), km::PageFlags::eAll);

    IsNotMapped(pt, range);

    // the remaining pages should still be mapped as 4k pages
    IsMapped(pt, mapping.last(x64::kLargePageSize * 2), km::PageFlags::eAll);
}

TEST_F(PageTableTest, PartialUnmapHead) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize);
    auto [vaddr, paddr, size] = mapping;

    IsMapped(pt, mapping, km::PageFlags::eAll);

    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // unmap the first 4k of the 2m page
    km::VirtualRange range = km::VirtualRange::of(vaddr, x64::kPageSize);
    OsStatus status = pt.unmap(range);
    ASSERT_EQ(OsStatusSuccess, status);

    // the first page should be unmaped, with the remaining pages still mapped
    IsNotMapped(pt, range);

    // the remaining pages should still be mapped as 4k pages
    IsMapped(pt, mapping.last(mapping.size - range.size()), km::PageFlags::eAll);
}

TEST_F(PageTableTest, PartialUnmapTail) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize);
    auto [vaddr, paddr, size] = mapping;

    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // unmap the first 4k of the 2m page
    auto tail = mapping.last(x64::kPageSize);
    km::VirtualRange range = tail.virtualRange();
    OsStatus status = pt.unmap(range);
    ASSERT_EQ(OsStatusSuccess, status);

    // the first page should be unmaped, with the remaining pages still mapped
    IsNotMapped(pt, range);

    // the remaining pages should still be mapped as 4k pages
    IsMapped(pt, mapping.first(mapping.size - range.size()), km::PageFlags::eAll);
}

TEST_F(PageTableTest, PartialUnmapMiddle) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize);
    auto [vaddr, paddr, size] = mapping;

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);
    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // unmap some 4k pages in the middle of the mapping
    const void *middle = (void*)((uintptr_t)vaddr + (x64::kPageSize * 8));
    km::VirtualRange range = km::VirtualRange::of(middle, x64::kPageSize);
    status = pt.unmap(range);
    ASSERT_EQ(OsStatusSuccess, status);

    // the first page should still be mapped
    IsMapped(pt, km::AddressMapping { vaddr, paddr, (x64::kPageSize * 8) }, km::PageFlags::eAll);

    // the middle page should be unmapped
    IsNotMapped(pt, range);

    // the remaining pages should still be mapped as 4k pages
    IsMapped(pt, km::AddressMapping { range.back, paddr + (x64::kPageSize * 8) + range.size(), mapping.size - (x64::kPageSize * 9) }, km::PageFlags::eAll);
}

TEST_F(PageTableTest, PartialUnmapAcrossPageBounds) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 4);
    auto [vaddr, paddr, size] = mapping;

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);
    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // unmap a range that crosses a 2m page boundary
    const void *middle = (void*)((uintptr_t)vaddr + (x64::kLargePageSize - (x64::kPageSize * 8)));
    uintptr_t offset = (uintptr_t)middle - (uintptr_t)vaddr;
    km::VirtualRange range = km::VirtualRange::of(middle, (x64::kPageSize * 16));
    status = pt.unmap(range);
    ASSERT_EQ(OsStatusSuccess, status);

    // the first page should still be mapped
    size_t headSize = offset;
    IsMapped(pt, km::AddressMapping { vaddr, paddr, headSize }, km::PageFlags::eAll);

    // the middle page should be unmapped
    IsNotMapped(pt, range);

    // the remaining pages should still be mapped as 4k pages
    size_t tailSize = mapping.size - offset - range.size();
    IsMapped(pt, km::AddressMapping { range.back, paddr + offset + range.size(), tailSize }, km::PageFlags::eAll);
}

TEST_F(PageTableTest, UnmapLargeRejectBadRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 4);
    auto [vaddr, paddr, size] = mapping;

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);
    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // should reject a range that is not 2m aligned
    const void *middle = (void*)((uintptr_t)vaddr + (x64::kLargePageSize - (x64::kPageSize * 8)));
    km::VirtualRange range = km::VirtualRange::of(middle, (x64::kPageSize * 16));
    status = pt.unmap2m(range);
    ASSERT_EQ(OsStatusInvalidInput, status);

    // mapping should still be valid
    IsMapped(pt, mapping, km::PageFlags::eAll);
}

TEST_F(PageTableTest, UnmapLargeRejectUnalignedTail) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 4);
    auto [vaddr, paddr, size] = mapping;

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);
    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // should reject a range that doesnt have a 2m aligned tail
    const void *middle = (void*)((uintptr_t)vaddr + x64::kLargePageSize);
    km::VirtualRange range = km::VirtualRange::of(middle, x64::kLargePageSize + (x64::kPageSize * 16));
    status = pt.unmap2m(range);
    ASSERT_EQ(OsStatusInvalidInput, status);

    // mapping should still be valid
    IsMapped(pt, mapping, km::PageFlags::eAll);
}

TEST_F(PageTableTest, UnmapLargeRejectSmallRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 4);
    auto [vaddr, paddr, size] = mapping;

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // should reject a range that isnt at least 2m large
    const void *middle = (void*)((uintptr_t)vaddr + x64::kLargePageSize);
    km::VirtualRange range = km::VirtualRange::of(middle, (x64::kPageSize * 16));
    status = pt.unmap2m(range);
    ASSERT_EQ(OsStatusInvalidInput, status);

    // mapping should still be valid
    IsMapped(pt, mapping, km::PageFlags::eAll);
}

TEST_F(PageTableTest, UnmapLargePageOnly) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 4);
    auto [vaddr, paddr, size] = mapping;

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    const void *middle = (void*)((uintptr_t)vaddr + x64::kLargePageSize);
    uintptr_t offset = (uintptr_t)middle - (uintptr_t)vaddr;
    km::VirtualRange range = km::VirtualRange::of(middle, x64::kLargePageSize);

    status = pt.unmap2m(range);
    ASSERT_EQ(OsStatusSuccess, status);

    // the first page should still be mapped
    size_t headSize = offset;
    IsMapped(pt, km::AddressMapping { vaddr, paddr, headSize }, km::PageFlags::eAll);

    // the middle page should be unmapped
    IsNotMapped(pt, range);

    // the remaining pages should still be mapped as 4k pages
    size_t tailSize = mapping.size - offset - range.size();
    IsMapped(pt, km::AddressMapping { range.back, paddr + offset + range.size(), tailSize }, km::PageFlags::eAll);
}

TEST_F(PageTableTest, UnmapLargePageOverSmallPages) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 4);
    auto [vaddr, paddr, size] = mapping;

    // test to ensure that unmapping a 2m range page of 4k pages works

    {
        OsStatus status = pt.map(mapping, km::PageFlags::eAll);
        ASSERT_EQ(OsStatusSuccess, status);
        IsMapped(pt, mapping, km::PageFlags::eAll);
        ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));
    }

    {
        // unmap a small region inside the large page to replace with 4k pages
        const void *middle = (void*)((uintptr_t)vaddr + x64::kPageSize);
        km::VirtualRange range = km::VirtualRange::of(middle, x64::kPageSize * 2);
        OsStatus status = pt.unmap(range);
        ASSERT_EQ(OsStatusSuccess, status);

        // verify
        IsMapped(pt, km::AddressMapping { vaddr, paddr, x64::kPageSize }, km::PageFlags::eAll);
        IsNotMapped(pt, range);
        IsMapped(pt, km::AddressMapping { range.back, paddr + x64::kPageSize * 3, x64::kPageSize }, km::PageFlags::eAll);
    }

    {
        // now perform a 2m unmap over the whole range
        OsStatus status = pt.unmap2m(mapping.virtualRange());
        ASSERT_EQ(OsStatusSuccess, status);

        // verify
        IsNotMapped(pt, mapping.virtualRange());
    }
}

// TODO: this test fails because the page table allocator will oom when theres just a little bit more than no memory left
// so the unmap will succeed despite the map failing with oom
#if 0
TEST_F(PageTableTest, UnmapOomHandling) {
    km::PageTables pt = ptes(km::PageFlags::eAll, x64::kPageSize * 8);

    // allocate pages to exhaust memory
    uintptr_t vbase = 0xFFFF800000000000;
    uintptr_t pbase = 0x1000000;
    while (true) {
        AddressMapping mapping = { (void*)vbase, pbase, x64::kLargePageSize };

        OsStatus status = pt.map(mapping, km::PageFlags::eAll);
        if (status == OsStatusOutOfMemory) {
            break;
        }

        vbase += x64::kLargePageSize;
        pbase += x64::kLargePageSize;
    }

    // now try to unmap the last 4k of the last 2m page
    const void *middle = (void*)(vbase - x64::kPageSize);
    km::VirtualRange range = km::VirtualRange::of(middle, x64::kPageSize);
    OsStatus status = pt.unmap(range);
    ASSERT_EQ(OsStatusOutOfMemory, status);

    // verify that the last 2m page is still mapped
    IsMapped(pt, km::AddressMapping { (void*)(vbase - x64::kLargePageSize), pbase - x64::kLargePageSize, x64::kLargePageSize }, km::PageFlags::eAll);
}
#endif

TEST_F(PageTableTest, UnmapUnmappedRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    // ensure that unmapping an unmapped range is a no-op
    km::VirtualRange range = km::VirtualRange::of((void*)0xFFFF800000000000, x64::kPageSize);
    OsStatus status = pt.unmap(range);
    ASSERT_EQ(OsStatusSuccess, status);
}

TEST_F(PageTableTest, UnmapUnmapped2mRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    // ensure that unmapping an unmapped range is a no-op
    km::VirtualRange range = km::VirtualRange::of((void*)0xFFFF800000000000, x64::kLargePageSize);
    OsStatus status = pt.unmap2m(range);
    ASSERT_EQ(OsStatusSuccess, status);
}

TEST_F(PageTableTest, GetBackingAddress) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);
    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MapPage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MapPageOffset) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 123));

    ASSERT_EQ(paddr.address + 123, addr.address);

    km::PageSize size = pt.getPageSize((void*)((uintptr_t)vaddr + 123));
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MapSmallRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 64;
    km::AddressMapping mapping { vaddr, paddr, pages * x64::kPageSize };
    OsStatus status = pt.map(PageMappingRequest{mapping, km::PageFlags::eAll, MemoryType::eWriteBack});
    ASSERT_EQ(OsStatusSuccess, status);

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = pt.getBackingAddress((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(paddr.address + (i * x64::kPageSize), addr.address);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eRegular, size);
    }
}

TEST_F(PageTableTest, MapLargePage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kLargePageSize };

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eLarge, size);
}

TEST_F(PageTableTest, MapLargePageOffset) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kLargePageSize };

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 12345));

    ASSERT_EQ(paddr.address + 12345, addr.address);

    km::PageSize size = pt.getPageSize((void*)((uintptr_t)vaddr + 12345));
    ASSERT_EQ(km::PageSize::eLarge, size);
}

#if 0
TEST_F(PageTableTest, MapHugePage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kHugePageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eHuge, size);
}

TEST_F(PageTableTest, MapHugePageOffset) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kHugePageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 1234567));

    ASSERT_EQ(paddr.address + 1234567, addr.address);

    km::PageSize size = pt.getPageSize((void*)((uintptr_t)vaddr + 1234567));
    ASSERT_EQ(km::PageSize::eHuge, size);
}
#endif

TEST_F(PageTableTest, MapRangeLarge) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    ASSERT_EQ(paddr.address % x64::kLargePageSize, 0);

    size_t pages = 1024;
    km::AddressMapping mapping { vaddr, paddr, pages * x64::kPageSize };
    OsStatus status = pt.map({mapping, km::PageFlags::eAll, MemoryType::eWriteBack});
    ASSERT_EQ(OsStatusSuccess, status);

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = pt.getBackingAddress((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(paddr.address + (i * x64::kPageSize), addr.address);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eLarge, size);
    }
}

TEST_F(PageTableTest, MemoryFlags) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    OsStatus status = pt.map(mapping, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PageFlags flags = pt.getMemoryFlags(vaddr);

    ASSERT_EQ(km::PageFlags::eAll, flags);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MemoryTypeWriteThrough) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    OsStatus status = pt.map(mapping, km::PageFlags::eRead, MemoryType::eWriteThrough);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PageFlags flags = pt.getMemoryFlags(vaddr);

    ASSERT_EQ(km::PageFlags::eRead, flags);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MemoryFlagsOnRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 64;
    km::AddressMapping mapping { vaddr, paddr, pages * x64::kPageSize };
    OsStatus status = pt.map({mapping, km::PageFlags::eAll, MemoryType::eWriteBack});
    ASSERT_EQ(OsStatusSuccess, status);

    for (size_t i = 0; i < pages; i++) {
        km::PageFlags flags = pt.getMemoryFlags((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageFlags::eAll, flags);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eRegular, size);
    }
}

TEST_F(PageTableTest, MemoryFlagsOnLargeRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    ASSERT_EQ(paddr.address % x64::kLargePageSize, 0);

    size_t pages = 1024;
    km::AddressMapping mapping { vaddr, paddr, pages * x64::kPageSize };
    OsStatus status = pt.map({mapping, km::PageFlags::eCode, MemoryType::eWriteBack});
    ASSERT_EQ(OsStatusSuccess, status);

    for (size_t i = 0; i < pages; i++) {
        km::PageFlags flags = pt.getMemoryFlags((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageFlags::eCode, flags);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eLarge, size);
    }
}

TEST_F(PageTableTest, MemoryFlagsOnNullMapping) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;

    km::PageFlags flags = pt.getMemoryFlags(vaddr);

    ASSERT_EQ(km::PageFlags::eNone, flags);
}

TEST_F(PageTableTest, PageSizeOnNullMapping) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;

    km::PageSize size = pt.getPageSize(vaddr);

    ASSERT_EQ(km::PageSize::eNone, size);
}

TEST_F(PageTableTest, MemoryFlagsKernelMapping) {
    uintptr_t kernel = 0x0000000007CC5000;
    uintptr_t hhdm = 0xFFFF800000000000;

    km::AddressMapping text = {
        .vaddr = (void*)0xFFFFFFFF80001000,
        .paddr = (0xFFFFFFFF80001000 - hhdm + kernel),
        .size = sm::roundup(0xFFFFFFFF8004101E - 0xFFFFFFFF80001000, x64::kPageSize),
    };

    km::AddressMapping rodata = {
        .vaddr = (void*)0xFFFFFFFF80042000,
        .paddr = (0xFFFFFFFF80042000 - hhdm + kernel),
        .size = sm::roundup(0xFFFFFFFF80047B88 - 0xFFFFFFFF80042000, x64::kPageSize),
    };

    km::AddressMapping data = {
        .vaddr = (void*)0xFFFFFFFF80048000,
        .paddr = (0xFFFFFFFF80048000 - hhdm + kernel),
        .size = sm::roundup(0xFFFFFFFF80049E40 - 0xFFFFFFFF80048000, x64::kPageSize),
    };

    km::PageTables pt = ptes(km::PageFlags::eAll);

    {
        OsStatus status = pt.map(text, km::PageFlags::eCode);
        ASSERT_EQ(OsStatusSuccess, status);
    }
    {
        OsStatus status = pt.map(rodata, km::PageFlags::eRead);
        ASSERT_EQ(OsStatusSuccess, status);
    }
    {
        OsStatus status = pt.map(data, km::PageFlags::eData);
        ASSERT_EQ(OsStatusSuccess, status);
    }

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

    // ensure that nothing else was mapped
    // scan 1mb above the kernel
    for (uintptr_t i = kernel - 0x100000; i < kernel + 0x100000; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eNone, flags);
    }

    // scan 1mb below the kernel
    for (uintptr_t i = hhdm - 0x100000; i < hhdm + 0x100000; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eNone, flags);
    }
}

TEST(MemoryRoundTest, RoundUp) {
    ASSERT_EQ(sm::roundup(0x1000, 0x1000), 0x1000);
    ASSERT_EQ(sm::roundup(0x1001, 0x1000), 0x2000);
    ASSERT_EQ(sm::roundup(0x1FFF, 0x1000), 0x2000);
    ASSERT_EQ(sm::roundup(0x2000, 0x1000), 0x2000);
    ASSERT_EQ(sm::roundup(0, 0x1000), 0);
}

TEST(MemoryRoundTest, RoundDown) {
    ASSERT_EQ(sm::rounddown(0x1000, 0x1000), 0x1000);
    ASSERT_EQ(sm::rounddown(0x1001, 0x1000), 0x1000);
    ASSERT_EQ(sm::rounddown(0x1FFF, 0x1000), 0x1000);
    ASSERT_EQ(sm::rounddown(0x2000, 0x1000), 0x2000);
    ASSERT_EQ(sm::rounddown(0, 0x1000), 0);
}

TEST(MemoryRoundTest, NextMultiple) {
    ASSERT_EQ(sm::nextMultiple(0, 0x1000), 0x1000);
    ASSERT_EQ(sm::nextMultiple(0x1000, 0x1000), 0x2000);
    ASSERT_EQ(sm::nextMultiple(0x100, 0x1000), 0x1000);
    ASSERT_EQ(sm::nextMultiple(0x1001, 0x1000), 0x2000);
    ASSERT_EQ(sm::nextMultiple(0x1FFF, 0x1000), 0x2000);
}

TEST_F(PageTableTest, AddressMapping) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    km::AddressMapping fb = {
        .vaddr = (void*)0xFFFFC00001000000,
        .paddr = 0x00000000FD000000,
        .size = 0x00000000FD3E8000 - 0x00000000FD000000,
    };

    OsStatus status = pt.map(fb, km::PageFlags::eData, km::MemoryType::eWriteCombine);
    ASSERT_EQ(OsStatusSuccess, status);

    km::PageFlags front = pt.getMemoryFlags(fb.vaddr);
    ASSERT_EQ(km::PageFlags::eData, front);

    km::PageFlags back = pt.getMemoryFlags((char*)fb.vaddr + fb.size - 1);
    ASSERT_EQ(km::PageFlags::eData, back);
}

TEST_F(PageTableTest, CompactTables) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto& allocator = pt.TESTING_getPageTableAllocator();
    auto before = allocator.stats();

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 8);
    auto [vaddr, paddr, size] = mapping;

    IsMapped(pt, mapping, km::PageFlags::eAll);
    ASSERT_EQ(km::PageSize::eLarge, pt.getPageSize(vaddr));

    // unmap the entire range in 4k increments
    for (size_t i = 0; i < size; i += x64::kPageSize) {
        km::VirtualRange range = km::VirtualRange::of((void*)((uintptr_t)vaddr + i), x64::kPageSize);
        OsStatus status = pt.unmap(range);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    auto after = allocator.stats();

    // compact
    size_t count = pt.compact();

    auto finished = allocator.stats();

    ASSERT_EQ(before.freeBlocks, finished.freeBlocks) << "compacting reclaimed free blocks";
    ASSERT_EQ(count, (before.freeBlocks - after.freeBlocks)) << "compacting returned the correct number of free blocks";
}

TEST_F(PageTableTest, RemapInLargePage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    auto mapping = MapArea(pt, 0xFFFF800000000000, 0x1000000, x64::kLargePageSize * 8);
    auto [vaddr, paddr, size] = mapping;

    // remapping a 4k area inside a 2m page should split the 2m page into 4k pages
    // the area not being remapped should still point to the same address but using 4k
    // pages instead of the 2m page

    km::VirtualRange range = km::VirtualRange::of((void*)((uintptr_t)vaddr + x64::kPageSize), x64::kPageSize);
    km::AddressMapping remap = { range.front, 0x00000000FFFF0000, x64::kPageSize };
    OsStatus status = pt.map(remap, km::PageFlags::eAll);
    ASSERT_EQ(OsStatusSuccess, status);

    // the remapped area should be mapped
    IsMapped(pt, remap, km::PageFlags::eAll);

    km::AddressMapping lower = { vaddr, paddr, x64::kPageSize };
    IsMapped(pt, lower, km::PageFlags::eAll);

    km::AddressMapping upper = { (void*)((uintptr_t)vaddr + x64::kPageSize * 2), paddr + x64::kPageSize * 2, size - (x64::kPageSize * 2) };
    IsMapped(pt, upper, km::PageFlags::eAll);
}

TEST_F(PageTableTest, TableLeaks) {
    auto pt = ptes(km::PageFlags::eAll, 64 * 0x1000);
    auto& alloc = pt.TESTING_getPageTableAllocator();

    {
        auto stats = alloc.stats();
        ASSERT_EQ(63, stats.freeBlocks);
    }

    {
        OsStatus status = pt.map(MappingOf(sm::VirtualAddress(0xFFFF800000000000), km::PhysicalAddressEx(0x1000), 0x1000 * 2), km::PageFlags::eAll);
        ASSERT_EQ(OsStatusSuccess, status);

        auto stats = alloc.stats();
        ASSERT_EQ(60, stats.freeBlocks);
    }
}
