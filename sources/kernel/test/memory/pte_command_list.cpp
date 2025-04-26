#include <gtest/gtest.h>

#include "memory/pte.hpp"
#include "memory/pt_command_list.hpp"
#include "setup.hpp"
#include "test/new_shim.hpp"

class PtCommandListTest : public testing::Test {
public:
    void SetUp() override {
        GetGlobalAllocator()->reset();
    }
};

TEST_F(PtCommandListTest, Construct) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[1024]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = 1024 * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };
    km::PageTableCommandList list { &pt };
}

TEST_F(PtCommandListTest, ValidateEmptyMapping) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[1024]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = 1024 * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);
        ASSERT_EQ(status, OsStatusInvalidSpan);

        auto listStats = list.stats();
        ASSERT_EQ(listStats.commandCount, 0);
        ASSERT_EQ(listStats.tableCount, 0);

        // invalid span should not be recorded
        ASSERT_EQ(list.validate(), OsStatusSuccess);
    }

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, ValidateEmptyUnmap) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[1024]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = 1024 * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.unmap(mapping.virtualRange());
        ASSERT_EQ(status, OsStatusInvalidSpan);


        auto listStats = list.stats();
        ASSERT_EQ(listStats.commandCount, 0);
        ASSERT_EQ(listStats.tableCount, 0);
        // invalid span should not be recorded
        ASSERT_EQ(list.validate(), OsStatusSuccess);
    }

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, RecordMap) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[1024]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = 1024 * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0x1000,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

    // the destructor will discard uncommited mappings
    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);
        ASSERT_EQ(status, OsStatusSuccess);

        auto listStats = list.stats();
        ASSERT_EQ(listStats.commandCount, 1);

        ASSERT_EQ(list.validate(), OsStatusSuccess);
    }

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMap) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[1024]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = 1024 * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0x1000,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);
        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_EQ(list.validate(), OsStatusSuccess);

        auto listStats = list.stats();
        ASSERT_EQ(listStats.commandCount, 1);

        list.commit();
    }

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr);

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMapMany) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[1024]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = 1024 * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::PageTableCommandList list { &pt };
    uintptr_t vbase = 0x1000;
    uintptr_t pbase = 0x2000;
    size_t count = 32;

    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);
    }

    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);
        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_EQ(list.validate(), OsStatusSuccess);
    }

    auto listStats = list.stats();
    ASSERT_EQ(listStats.commandCount, count);

    list.commit();

    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr);
    }

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMapManyPteOom) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    size_t pteCount = 16;
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[pteCount]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = pteCount * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::PageTableCommandList list { &pt };
    uintptr_t vbase = 0x1000;
    uintptr_t pbase = 0x2000;
    size_t count = 64;

    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);
    }

    size_t last = 0;
    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

        auto inner0 = pt.TESTING_getPageTableAllocator().stats();
        auto list0 = list.stats();

        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);

        auto inner1 = pt.TESTING_getPageTableAllocator().stats();
        auto list1 = list.stats();

        if (status == OsStatusOutOfMemory) {
            ASSERT_EQ(inner0.freeBlocks, inner1.freeBlocks);
            ASSERT_EQ(list0.commandCount, list1.commandCount);
            ASSERT_EQ(list0.tableCount, list1.tableCount);
            break;
        }

        last = i;

        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_EQ(list.validate(), OsStatusSuccess);
    }

    ASSERT_EQ(list.validate(), OsStatusSuccess);

    auto listStats = list.stats();
    ASSERT_EQ(listStats.commandCount, last + 1);

    // on oom the previously recorded mappings are not discarded
    list.commit();

    ASSERT_NE(count, last);
    ASSERT_NE(0, last);

    for (size_t i = 0; i < last + 1; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr);
    }

    for (size_t i = last + 1; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY) << "vaddr: " << mapping.vaddr;
    }

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMapManyListOom) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    size_t pteCount = 1024;
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[pteCount]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = pteCount * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::PageTableCommandList list { &pt };
    uintptr_t vbase = 0x1000;
    uintptr_t pbase = 0x2000;
    size_t count = 64;

    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);
    }

    size_t last = 0;
    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);

        GetGlobalAllocator()->mFailAfter = 0;
        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);
        GetGlobalAllocator()->mFailAfter = SIZE_MAX;

        if (status == OsStatusOutOfMemory) {
            break;
        }

        last = i;
        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_EQ(list.validate(), OsStatusSuccess);
    }

    ASSERT_EQ(list.validate(), OsStatusSuccess);

    auto listStats = list.stats();
    ASSERT_EQ(listStats.commandCount, last + 1);

    // on oom the previously recorded mappings are not discarded
    list.commit();

    ASSERT_NE(count, last);
    ASSERT_NE(0, last);

    for (size_t i = 0; i < last + 1; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr);
    }

    for (size_t i = last + 1; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY) << "vaddr: " << mapping.vaddr;
    }

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, RecordUnmap) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    size_t pteCount = 1024;
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[pteCount]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = pteCount * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.unmap(km::VirtualRange::of((void*)0x1000, 0x2000));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitUnmap) {
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    size_t pteCount = 1024;
    std::unique_ptr<x64::page[]> pteMemory(new x64::page[pteCount]);
    km::AddressMapping pteMapping {
        .vaddr = std::bit_cast<void*>(pteMemory.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
        .size = pteCount * sizeof(x64::page),
    };
    km::PageTables pt { &pm, pteMapping, km::PageFlags::eUserAll };

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0x1000,
    };

    ASSERT_EQ(pt.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack), OsStatusSuccess);

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr);

    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.unmap(km::VirtualRange::of((void*)0x1000, 0x2000));
        ASSERT_EQ(status, OsStatusSuccess);

        list.commit();
    }

    (void)pt.compact();
    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), KM_INVALID_MEMORY);
}
