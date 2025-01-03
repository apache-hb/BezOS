#include <gtest/gtest.h>

#include "memory/paging.hpp"

TEST(PageManagerTest, MemoryTypeIndex) {
    km::PageMemoryTypeLayout layout = {
        .deferred = 0,
        .uncached = 1,
        .writeCombined = 2,
        .writeThrough = 3,
        .writeProtect = 4,
        .writeBack = 5,
    };

    km::PageManager pm = { 40, 0, layout };

    x64::pte pte;
    pm.setMemoryType(pte, km::MemoryType::eUncached);

    ASSERT_EQ(pte.memoryType(), layout.uncached);
}

TEST(PageManagerTest, PdeMemoryType) {
    km::PageMemoryTypeLayout layout = {
        .deferred = 0,
        .uncached = 1,
        .writeCombined = 2,
        .writeThrough = 3,
        .writeProtect = 4,
        .writeBack = 5,
    };

    km::PageManager pm = { 40, 0, layout };

    x64::pde pte;
    pm.setMemoryType(pte, km::MemoryType::eUncached);

    ASSERT_EQ(pte.memoryType(), layout.uncached);
}

TEST(PageManagerTest, PdpteMemoryType) {
    km::PageMemoryTypeLayout layout = {
        .deferred = 0,
        .uncached = 1,
        .writeCombined = 2,
        .writeThrough = 3,
        .writeProtect = 4,
        .writeBack = 5,
    };

    km::PageManager pm = { 40, 0, layout };

    x64::pdpte pte;
    pm.setMemoryType(pte, km::MemoryType::eUncached);

    ASSERT_EQ(pte.memoryType(), layout.uncached);
}

TEST(PageManagerTest, PteEntryMemoryType) {
    for (uint8_t i = 0; i < 6; i++) {
        x64::pte pte;
        pte.setPatEntry(i);

        ASSERT_EQ(pte.memoryType(), i);
    }
}

TEST(PageManagerTest, PdeEntryMemoryType) {
    for (uint8_t i = 0; i < 6; i++) {
        x64::pde pde;
        pde.setPatEntry(i);

        ASSERT_EQ(pde.memoryType(), i);
    }
}

TEST(PageManagerTest, PdpteEntryMemoryType) {
    for (uint8_t i = 0; i < 6; i++) {
        x64::pdpte pdpte;
        pdpte.setPatEntry(i);

        ASSERT_EQ(pdpte.memoryType(), i);
    }
}
