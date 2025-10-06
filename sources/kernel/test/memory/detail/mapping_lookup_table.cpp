#include <gtest/gtest.h>

#include "memory/detail/mapping_lookup_table.hpp"

TEST(MappingLookupCacheConstructTest, Construct) {
    char storage[0x1000];
    km::detail::MappingLookupTable cache{storage, sizeof(storage)};
}

class MappingLookupCacheTest : public testing::Test {
public:
    char storage[0x1000];
    km::detail::MappingLookupTable cache;

    void SetUp() override {
        cache = km::detail::MappingLookupTable{storage, sizeof(storage)};
    }
};

TEST_F(MappingLookupCacheTest, AddAndFind) {
    km::AddressMapping mapping {
        .vaddr = (void*)0x200000,
        .paddr = 0x100000,
        .size = 0x3000
    };

    ASSERT_EQ(cache.addMemory(mapping), OsStatusSuccess);

    sm::VirtualAddress vaddr = cache.find(0x100000);
    ASSERT_EQ(vaddr, mapping.vaddr);

    for (size_t i = 0; i < mapping.size; i += x64::kPageSize) {
        sm::VirtualAddress vaddr = cache.find(0x100000 + i);
        EXPECT_EQ(vaddr.address, (uintptr_t)mapping.vaddr + i) << std::format("Failed at offset {:#x}", 0x100000 + i);
    }

    sm::VirtualAddress invalid = cache.find(0x100000 - x64::kPageSize);
    EXPECT_EQ(invalid, sm::VirtualAddress::invalid());

    invalid = cache.find(mapping.paddr.address + mapping.size + 1);
    EXPECT_EQ(invalid, sm::VirtualAddress::invalid());
}
