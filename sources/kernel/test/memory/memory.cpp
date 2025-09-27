#include <gtest/gtest.h>

#include "memory/memory.hpp"

using namespace km;

TEST(MemoryDetailTest, LargePageAligned) {
    AddressMapping aligned2m {
        .vaddr = (void*)x64::kLargePageSize,
        .paddr = x64::kLargePageSize,
        .size = x64::kLargePageSize,
    };

    //
    // Mappings that are 2m aligned are elegible for large pages.
    //
    ASSERT_TRUE(detail::IsLargePageEligible(aligned2m));

    AddressMapping smallerThan2m {
        .vaddr = (void*)(x64::kLargePageSize * 0x10000),
        .paddr = x64::kLargePageSize,
        .size = x64::kLargePageSize - 1,
    };

    //
    // Mappings that are smaller than 2m are not elegible for large pages.
    //
    ASSERT_FALSE(detail::IsLargePageEligible(smallerThan2m));

    AddressMapping inequalAlignment {
        .vaddr = (void*)(x64::kLargePageSize * 0x10000),
        .paddr = x64::kLargePageSize + 0x1000,
        .size = x64::kLargePageSize,
    };

    //
    // Mappings that are not aligned equally relative to the large page size
    // are not elegible for large pages.
    //

    ASSERT_FALSE(detail::IsLargePageEligible(inequalAlignment));

    AddressMapping smallerThan2mAfterAlignment {
        .vaddr = (void*)(x64::kLargePageSize + 0xF0000),
        .paddr = x64::kLargePageSize + 0xF0000,
        .size = x64::kLargePageSize + 0x100000,
    };

    //
    // Mappings that are smaller than 2m after alignment are not elegible for large pages.
    //

    ASSERT_FALSE(detail::IsLargePageEligible(smallerThan2mAfterAlignment));

    AddressMapping largerThan2mAfterAlignment {
        .vaddr = (void*)(x64::kLargePageSize + 0xF0000),
        .paddr = x64::kLargePageSize + 0xF0000,
        .size = x64::kLargePageSize * 3,
    };

    //
    // Mappings that are still larger than 2m after alignment are elegible for large pages.
    //

    ASSERT_TRUE(detail::IsLargePageEligible(largerThan2mAfterAlignment));

    AddressMapping equalAlignment {
        .vaddr = (void*)(x64::kLargePageSize + 0xF0000),
        .paddr = x64::kLargePageSize + 0xF0000,
        .size = x64::kLargePageSize * 2,
    };

    //
    // Mappings that are aligned equally relative to the large page size
    // and are larger than 2m after alignment are elegible for large pages.
    //

    ASSERT_TRUE(detail::IsLargePageEligible(equalAlignment));
}

TEST(MemoryDetailTest, AlignLargeRangeSimple) {
    AddressMapping mapping {
        .vaddr = (void*)x64::kLargePageSize,
        .paddr = x64::kLargePageSize,
        .size = x64::kLargePageSize,
    };

    AddressMapping head = detail::AlignLargeRangeHead(mapping);
    AddressMapping body = detail::AlignLargeRangeBody(mapping);
    AddressMapping tail = detail::AlignLargeRangeTail(mapping);

    ASSERT_TRUE(head.isEmpty());
    ASSERT_FALSE(body.isEmpty());
    ASSERT_TRUE(tail.isEmpty());

    ASSERT_EQ(body.vaddr, (void*)x64::kLargePageSize);
    ASSERT_EQ(body.paddr, x64::kLargePageSize);
    ASSERT_EQ(body.size, x64::kLargePageSize);
}

TEST(MemoryDetailTest, AlignLargeRangeOffsetAlignment) {
    AddressMapping mapping {
        .vaddr = (void*)(x64::kLargePageSize - 0x1000),
        .paddr = x64::kLargePageSize - 0x1000,
        .size = x64::kLargePageSize * 2,
    };

    AddressMapping head = detail::AlignLargeRangeHead(mapping);
    AddressMapping body = detail::AlignLargeRangeBody(mapping);
    AddressMapping tail = detail::AlignLargeRangeTail(mapping);

    ASSERT_FALSE(head.isEmpty());
    ASSERT_FALSE(body.isEmpty());
    ASSERT_FALSE(tail.isEmpty());

    ASSERT_EQ(head.vaddr, mapping.vaddr);
    ASSERT_EQ(head.paddr, mapping.paddr);
    ASSERT_EQ(head.size, 0x1000);

    ASSERT_EQ(body.vaddr, (void*)(x64::kLargePageSize));
    ASSERT_EQ(body.paddr, x64::kLargePageSize);
    ASSERT_EQ(body.size, x64::kLargePageSize);

    ASSERT_EQ(tail.vaddr, (void*)(x64::kLargePageSize * 2));
    ASSERT_EQ(tail.paddr, x64::kLargePageSize * 2);
    ASSERT_EQ(tail.size, 0x1FF000);
}

TEST(MemoryDetailTest, AlignLargeRangeSinglePage) {
    AddressMapping mapping {
        .vaddr = (void*)(x64::kLargePageSize - 0x1000),
        .paddr = x64::kLargePageSize - 0x1000,
        .size = x64::kLargePageSize + 0x1000,
    };

    AddressMapping head = detail::AlignLargeRangeHead(mapping);
    AddressMapping body = detail::AlignLargeRangeBody(mapping);
    AddressMapping tail = detail::AlignLargeRangeTail(mapping);

    ASSERT_FALSE(head.isEmpty());
    ASSERT_FALSE(body.isEmpty());
    ASSERT_TRUE(tail.isEmpty());

    ASSERT_EQ(head.vaddr, mapping.vaddr);
    ASSERT_EQ(head.paddr, mapping.paddr);
    ASSERT_EQ(head.size, 0x1000);

    ASSERT_EQ(body.vaddr, (void*)(x64::kLargePageSize));
    ASSERT_EQ(body.paddr, x64::kLargePageSize);
    ASSERT_EQ(body.size, x64::kLargePageSize);
}

TEST(MemoryDetailTest, AlignLargeRangeWithTail) {
    AddressMapping mapping {
        .vaddr = (void*)(x64::kLargePageSize ),
        .paddr = x64::kLargePageSize,
        .size = x64::kLargePageSize + 0x1000,
    };

    AddressMapping head = detail::AlignLargeRangeHead(mapping);
    AddressMapping body = detail::AlignLargeRangeBody(mapping);
    AddressMapping tail = detail::AlignLargeRangeTail(mapping);

    ASSERT_TRUE(head.isEmpty());
    ASSERT_FALSE(body.isEmpty());
    ASSERT_FALSE(tail.isEmpty());

    ASSERT_EQ(body.vaddr, (void*)(x64::kLargePageSize));
    ASSERT_EQ(body.paddr, x64::kLargePageSize);
    ASSERT_EQ(body.size, x64::kLargePageSize);

    ASSERT_EQ(tail.vaddr, (void*)(x64::kLargePageSize * 2));
    ASSERT_EQ(tail.paddr, x64::kLargePageSize * 2);
    ASSERT_EQ(tail.size, 0x1000);
}

TEST(MemoryDetailTest, AddressParts) {
    uintptr_t address = 0xFFFF800000000000;
    PageWalkIndices indices = getAddressParts(address);

    ASSERT_EQ(indices.pml4e, 256);
    ASSERT_EQ(indices.pdpte, 0x0);
    ASSERT_EQ(indices.pdte, 0x0);
    ASSERT_EQ(indices.pte, 0x0);
}

TEST(MemoryDetailTest, CoveredSegmentsPml4) {
    VirtualRange range = { (void*)0xFFFF'8000'0000'0000, (void*)0xFFFF'C000'0000'0000 };
    size_t segments = detail::GetCoveredSegments(range, (x64::kHugePageSize * 512));

    ASSERT_EQ(segments, 128);
}

TEST(MemoryDetailTest, CoveredSegmentsPml4Spill) {
    VirtualRange range = { (void*)0xFFFF'8000'0000'0000, (void*)0xFFFF'C000'0000'0001 };
    size_t segments = detail::GetCoveredSegments(range, (x64::kHugePageSize * 512));

    ASSERT_EQ(segments, 129);
}

static size_t GetMaxPages(uintptr_t front, uintptr_t back) {
    VirtualRange range = { (void*)front, (void*)back };
    return detail::MaxPagesForMapping(range);
}

TEST(MemoryDetailTest, MaxPagesSmallMapping) {
    size_t pages = GetMaxPages(0x1000, 0x2000);

    // pml4, pdpt, pd, pt
    ASSERT_EQ(pages, 1 + 1 + 1 + 1);
}

TEST(MemoryDetailTest, MaxPagesSmallMappingSpill) {
    size_t pages = GetMaxPages(x64::kLargePageSize - 0x1000, x64::kLargePageSize + 0x1000);

    // pml4, pdpt, pd, pt
    ASSERT_EQ(pages, 1 + 1 + 2 + 2);
}

TEST(MemoryDetailTest, MaxPages2mMapping) {
    size_t pages = GetMaxPages(x64::kLargePageSize, x64::kLargePageSize * 2);

    // pml4, pdpt, pd, pt
    ASSERT_EQ(pages, 1 + 1 + 1);
}

TEST(MemoryDetailTest, MaxPages2mMappingSpill) {
    size_t pages = GetMaxPages(x64::kLargePageSize, (x64::kLargePageSize * 2) + 1);

    // pml4, pdpt, pd, pt
    ASSERT_EQ(pages, 1 + 1 + 2 + 1);
}

TEST(MemoryDetailTest, SpillPml4) {
    size_t pages = GetMaxPages(0xFFFF'8000'0000'0000, 0xFFFF'C000'0000'0001);
    size_t pml4e = 128;
    size_t pdpte = pml4e * 512;
    size_t pdte = pdpte * 512;

    // one extra pml4e, pdpt, pd, and pt
    ASSERT_EQ(pages, pml4e + pdpte + pdte + 1 + 1 + 1 + 1);
}
