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
    PageWalkIndices indices = GetAddressParts(address);

    ASSERT_EQ(indices.pml4e, 256);
    ASSERT_EQ(indices.pdpte, 0x0);
    ASSERT_EQ(indices.pdte, 0x0);
    ASSERT_EQ(indices.pte, 0x0);
}

class PageTablesTest : public testing::Test {
public:
    void SetUp() override {
        buffer = new (kPteMemoryAlignment) std::byte[kPteMemorySize];
    }

    void TearDown() override {
        operator delete[](buffer, kPteMemoryAlignment);
    }

    static constexpr size_t kPteMemorySize = x64::kPageSize * 64;
    static constexpr std::align_val_t kPteMemoryAlignment = std::align_val_t(x64::kPageSize);
    std::byte *buffer;

    AddressMapping pteMemory() {
        return AddressMapping { (void*)buffer, (uintptr_t)buffer, kPteMemorySize };
    }
    km::PageBuilder pm { 48, 48, 0, km::PageMemoryTypeLayout { 2, 3, 4, 1, 5, 0 } };
};

TEST_F(PageTablesTest, Construct) {
    PageTables pt { pteMemory(), &pm };

    ASSERT_NE(pt.root(), nullptr);
}

TEST_F(PageTablesTest, MapMemory) {
    PageTables pt { pteMemory(), &pm };

    ASSERT_NE(pt.root(), nullptr);

    MappingRequest request {
        .mapping = {
            .vaddr = (void*)0xFFFF800000000000,
            .paddr = 0x1000000,
            .size = x64::kPageSize,
        },
        .flags = PageFlags::eAll,
        .type = MemoryType::eWriteBack,
    };

    ASSERT_EQ(pt.map(request), OsStatusSuccess);

    PageWalk walk;
    ASSERT_EQ(pt.walk(request.mapping.vaddr, &walk), OsStatusSuccess);

    ASSERT_EQ(pt.getBackingAddress(request.mapping.vaddr), request.mapping.paddr);

    ASSERT_EQ(walk.pml4eIndex, 256);
    ASSERT_EQ(walk.pdpteIndex, 0);
    ASSERT_EQ(walk.pdteIndex, 0);
    ASSERT_EQ(walk.pteIndex, 0);

    ASSERT_TRUE(walk.pml4e.present());
    ASSERT_TRUE(walk.pdpte.present());
    ASSERT_TRUE(walk.pdte.present());
    ASSERT_TRUE(walk.pte.present());

    ASSERT_EQ(walk.pageSize(), PageSize2::eRegular);
}

TEST_F(PageTablesTest, MapLargeMemory) {
    PageTables pt { pteMemory(), &pm };

    ASSERT_NE(pt.root(), nullptr);

    MappingRequest request {
        .mapping = {
            .vaddr = (void*)0xFFFF800000000000,
            .paddr = 0x1000000,
            .size = x64::kLargePageSize * 16,
        },
        .flags = PageFlags::eAll,
        .type = MemoryType::eWriteBack,
    };

    ASSERT_EQ(pt.map(request), OsStatusSuccess);

    PageWalk walk;
    ASSERT_EQ(pt.walk(request.mapping.vaddr, &walk), OsStatusSuccess);

    ASSERT_EQ(pt.getBackingAddress(request.mapping.vaddr), request.mapping.paddr);

    ASSERT_EQ(walk.pml4eIndex, 256);
    ASSERT_EQ(walk.pdpteIndex, 0);
    ASSERT_EQ(walk.pdteIndex, 0);
    ASSERT_EQ(walk.pteIndex, 0);

    ASSERT_TRUE(walk.pml4e.present());
    ASSERT_TRUE(walk.pdpte.present());
    ASSERT_TRUE(walk.pdte.is2m());
    ASSERT_FALSE(walk.pte.present());

    ASSERT_EQ(walk.pageSize(), PageSize2::eLarge);
}

class PageTableFlagsTest :
    public PageTablesTest,
    public testing::WithParamInterface<std::tuple<PageFlags, MemoryType>>
{ };

INSTANTIATE_TEST_SUITE_P(PageTableFlags, PageTableFlagsTest,
    testing::Combine(
        testing::Values(
            PageFlags::eRead,
            PageFlags::eData,
            PageFlags::eUserData,
            PageFlags::eCode,
            PageFlags::eUserCode,
            PageFlags::eAll
        ),
        testing::Values(
            MemoryType::eWriteBack,
            MemoryType::eWriteThrough,
            MemoryType::eUncached
        )
    ));

TEST_P(PageTableFlagsTest, PageFlags) {
    auto [flags, type] = GetParam();

    PageTables pt { pteMemory(), &pm };

    ASSERT_NE(pt.root(), nullptr);

    MappingRequest request {
        .mapping = {
            .vaddr = (void*)0xFFFF800000000000,
            .paddr = 0x1000000,
            .size = x64::kPageSize,
        },
        .flags = flags,
        .type = type,
    };

    ASSERT_EQ(pt.map(request), OsStatusSuccess);

    ASSERT_EQ(pt.getMemoryFlags(request.mapping.vaddr), flags);
    ASSERT_EQ(pt.getPageSize(request.mapping.vaddr), PageSize2::eRegular);

    PageWalk walk;
    ASSERT_EQ(pt.walk(request.mapping.vaddr, &walk), OsStatusSuccess);

    ASSERT_EQ(walk.pageSize(), PageSize2::eRegular);
    ASSERT_EQ(walk.flags(), flags);
}

TEST_P(PageTableFlagsTest, PageFlagsLarge) {
    auto [flags, type] = GetParam();

    PageTables pt { pteMemory(), &pm };

    ASSERT_NE(pt.root(), nullptr);

    MappingRequest request {
        .mapping = {
            .vaddr = (void*)0xFFFF800000000000,
            .paddr = 0x1000000,
            .size = x64::kLargePageSize * 16,
        },
        .flags = flags,
        .type = type,
    };

    ASSERT_EQ(pt.map(request), OsStatusSuccess);

    ASSERT_EQ(pt.getMemoryFlags(request.mapping.vaddr), flags);
    ASSERT_EQ(pt.getPageSize(request.mapping.vaddr), PageSize2::eLarge);

    PageWalk walk;
    ASSERT_EQ(pt.walk(request.mapping.vaddr, &walk), OsStatusSuccess);

    ASSERT_EQ(walk.pageSize(), PageSize2::eLarge);
    ASSERT_EQ(walk.flags(), flags);
}
