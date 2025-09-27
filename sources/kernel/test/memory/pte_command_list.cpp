#include <gtest/gtest.h>

#include "memory/page_tables.hpp"
#include "memory/pt_command_list.hpp"
#include "setup.hpp"
#include "new_shim.hpp"

static constexpr size_t kPtCount = 1024;

class PtCommandListConstructTest : public testing::Test {
public:
    void SetUp() override {
        GetGlobalAllocator()->reset();
        setup(kPtCount);
    }

    void setup(size_t ptCount) {
        pteMemory.reset(new x64::page[ptCount]);
        km::AddressMapping pteMapping {
            .vaddr = std::bit_cast<void*>(pteMemory.get()),
            .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory.get()),
            .size = ptCount * sizeof(x64::page),
        };
        OsStatus status = km::PageTables::create(&pm, pteMapping, km::PageFlags::eUserAll, &pt);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
    std::unique_ptr<x64::page[]> pteMemory;
    km::PageTables pt;
};

TEST_F(PtCommandListConstructTest, Construct) {
    km::PageTableCommandList list { &pt };
}

class PtCommandListTest : public PtCommandListConstructTest {
public:
    void SetUp() override {
        PtCommandListConstructTest::SetUp();
    }
};

TEST_F(PtCommandListTest, ValidateEmptyMapping) {
    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

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

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, ValidateEmptyUnmap) {
    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

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

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, RecordMap) {
    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0x1000,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

    // the destructor will discard uncommited mappings
    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);
        ASSERT_EQ(status, OsStatusSuccess);

        auto listStats = list.stats();
        ASSERT_EQ(listStats.commandCount, 1);

        ASSERT_EQ(list.validate(), OsStatusSuccess);
    }

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMap) {
    auto stats0 = pt.TESTING_getPageTableAllocator().stats();

    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0x1000,
    };

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack);
        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_EQ(list.validate(), OsStatusSuccess);

        auto listStats = list.stats();
        ASSERT_EQ(listStats.commandCount, 1);

        list.commit();
    }

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr.address);

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMapMany) {
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

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());
    }

    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

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

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr.address);
    }

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMapManyPteOom) {
    setup(16);

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

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());
    }

    size_t last = 0;
    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

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

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr.address);
    }

    for (size_t i = last + 1; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid()) << "vaddr: " << mapping.vaddr;
    }

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, CommitMapManyListOom) {
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

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());
    }

    size_t last = 0;
    for (size_t i = 0; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());

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

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr.address);
    }

    for (size_t i = last + 1; i < count; i++) {
        km::AddressMapping mapping {
            .vaddr = (void*)(vbase + (i * 0x1000)),
            .paddr = (pbase + (i * 0x1000)),
            .size = 0x1000,
        };

        ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid()) << "vaddr: " << mapping.vaddr;
    }

    auto stats1 = pt.TESTING_getPageTableAllocator().stats();
    ASSERT_NE(stats1.freeBlocks, stats0.freeBlocks);
}

TEST_F(PtCommandListTest, RecordUnmap) {
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
    km::AddressMapping mapping {
        .vaddr = (void*)0x1000,
        .paddr = 0x2000,
        .size = 0x1000,
    };

    ASSERT_EQ(pt.map(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack), OsStatusSuccess);

    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), mapping.paddr.address);

    {
        km::PageTableCommandList list { &pt };

        OsStatus status = list.unmap(km::VirtualRange::of((void*)0x1000, 0x2000));
        ASSERT_EQ(status, OsStatusSuccess);

        list.commit();
    }

    (void)pt.compact();
    ASSERT_EQ(pt.getBackingAddress(mapping.vaddr), km::PhysicalAddressEx::invalid());
}
