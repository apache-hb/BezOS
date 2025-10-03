#include <gtest/gtest.h>

#include "memory/detail/mapping_lookup_cache.hpp"

TEST(MappingLookupCacheConstructTest, Construct) {
    char storage[0x1000];
    km::detail::MappingLookupCache cache{storage, sizeof(storage)};
}

class MappingLookupCacheTest : public testing::Test {
public:
    char storage[0x1000];
    km::detail::MappingLookupCache cache;

    void SetUp() override {
        cache = km::detail::MappingLookupCache{storage, sizeof(storage)};
    }
};

TEST_F(MappingLookupCacheTest, AddAndFind) {
    km::AddressMapping mapping {
        .vaddr = (void*)0x200000,
        .paddr = 0x100000,
        .size = 0x3000
    };

    ASSERT_EQ(cache.addMemory(mapping), OsStatusSuccess);

    sm::PhysicalAddress paddr = cache.find(0x200000);
    ASSERT_EQ(paddr.address, mapping.paddr.address);

    for (size_t i = 0; i < mapping.size; i += x64::kPageSize) {
        sm::PhysicalAddress paddr = cache.find(0x200000 + i);
        EXPECT_EQ(paddr.address, mapping.paddr.address + i) << std::format("Failed at offset {:#x}", 0x200000 + i);
    }

    sm::PhysicalAddress invalid = cache.find(0x200000 - x64::kPageSize);
    EXPECT_EQ(invalid, sm::PhysicalAddress::invalid());

    invalid = cache.find((uintptr_t)mapping.vaddr + mapping.size + 1);
    EXPECT_EQ(invalid, sm::PhysicalAddress::invalid());
}
