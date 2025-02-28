#include <gtest/gtest.h>

#include "memory/layout.hpp"

using namespace km;

TEST(MappingTest, Construct) {
    [[maybe_unused]] AddressMapping mapping;
}

TEST(MappingTest, ZeroSlide) {
    AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = PhysicalAddress(0x1000),
        .size = 0x1000,
    };

    EXPECT_EQ(mapping.slide(), 0);
}

TEST(MappingTest, NegativeSlide) {
    AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = PhysicalAddress(0x2000),
        .size = 0x1000,
    };

    EXPECT_EQ(mapping.slide(), -0x1000);
}

TEST(MappingTest, PositiveSlide) {
    AddressMapping mapping {
        .vaddr = (void*)0x2000,
        .paddr = PhysicalAddress(0x1000),
        .size = 0x1000,
    };

    EXPECT_EQ(mapping.slide(), 0x1000);
}

static void TestEqualOffsetTo(uintptr_t vaddr, uintptr_t paddr, size_t size, size_t align) {
    AddressMapping mapping {
        .vaddr = (void*)vaddr,
        .paddr = PhysicalAddress(paddr),
        .size = size,
    };

    EXPECT_TRUE(mapping.equalOffsetTo(align));
}

TEST(MappingTest, EqualOffsetTo) {
    TestEqualOffsetTo(0x1000, 0x1000, 0x1000, x64::kPageSize);
    TestEqualOffsetTo(0x1100, 0x2100, 0x1000, x64::kPageSize);
    TestEqualOffsetTo(0x2000, 0x1000, 0x1000, x64::kPageSize);
    TestEqualOffsetTo(0x2FFF, 0x3FFF, 0x1000, x64::kPageSize);
}

TEST(MappingTest, AlignOut) {
    VirtualRange vrange = { (void*)0x1100, (void*)0x2100 };
    VirtualRange aligned = alignedOut(vrange, x64::kPageSize);
    EXPECT_EQ(aligned.front, (void*)0x1000);
    EXPECT_EQ(aligned.back, (void*)0x3000);
}

TEST(MappingTest, Align) {
    AddressMapping mapping {
        .vaddr = (void*)0x1100,
        .paddr = PhysicalAddress(0x1100),
        .size = 0x1000,
    };

    AddressMapping other = mapping.aligned(x64::kPageSize);
    ASSERT_EQ(other.vaddr, (void*)0x1000);
    ASSERT_EQ(other.paddr, PhysicalAddress(0x1000));

    // 0x2000 rather than 0x1000 as the mapping needs to cover the entire original range.
    ASSERT_EQ(other.size, 0x2000);
}

TEST(MappingTest, Align2) {
    AddressMapping mapping {
        .vaddr = (void*)0xFFFFFFFF80001000,
        .paddr = PhysicalAddress(0x0000000007253000),
        .size = sm::megabytes(1).bytes(),
    };

    AddressMapping other = mapping.aligned(x64::kPageSize);
    ASSERT_EQ(other.vaddr, (void*)0xFFFFFFFF80001000);
    ASSERT_EQ(other.paddr, PhysicalAddress(0x0000000007253000));
    ASSERT_EQ(other.size, 0x100000);
}
