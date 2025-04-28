#include <gtest/gtest.h>

#include "system/vmm.hpp"
#include "setup.hpp"
#include "system/pmm.hpp"

class AddressSpaceManagerTest : public testing::Test {
public:
    void SetUp() override {
        pteMemory0.reset(new x64::page[1024]);
        pteMemory1.reset(new x64::page[1024]);
    }

    void TearDown() override {

    }

    km::AddressMapping getPteMapping0() const {
        return km::AddressMapping {
            .vaddr = std::bit_cast<const void*>(pteMemory0.get()),
            .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory0.get()),
            .size = 1024 * sizeof(x64::page),
        };
    }

    km::AddressMapping getPteMapping1() const {
        return km::AddressMapping {
            .vaddr = std::bit_cast<const void*>(pteMemory1.get()),
            .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory1.get()),
            .size = 1024 * sizeof(x64::page),
        };
    }

    std::unique_ptr<x64::page[]> pteMemory0;
    std::unique_ptr<x64::page[]> pteMemory1;
    sys2::MemoryManager memory;
    sys2::AddressSpaceManager asManager0;
    sys2::AddressSpaceManager asManager1;
    km::PageBuilder pager { 48, 48, km::GetDefaultPatLayout() };
};

TEST_F(AddressSpaceManagerTest, Construct) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 0);
}

TEST_F(AddressSpaceManagerTest, Allocate) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x1000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    status = asManager0.unmap(&memory, mapping.virtualRange());
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapMiddle) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr + 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x3000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.querySegment(subrange.back, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.querySegment((void*)((uintptr_t)mapping.vaddr + 0x2000), &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 2);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 2);
}

TEST_F(AddressSpaceManagerTest, UnmapExact) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr), (void*)((uintptr_t)mapping.vaddr + 0x4000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    status = asManager0.querySegment(subrange.back, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    status = asManager0.querySegment((void*)((uintptr_t)mapping.vaddr + 0x2000), &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 0);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapFront) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { mapping.vaddr, (void*)((uintptr_t)mapping.vaddr + 0x2000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    status = asManager0.querySegment(subrange.back, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 1);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 1);
}

TEST_F(AddressSpaceManagerTest, UnmapBack) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr + 0x2000), (void*)((uintptr_t)mapping.vaddr + 0x4000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 1);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 1);
}

TEST_F(AddressSpaceManagerTest, UnmapSpill) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr - 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x5000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 0);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapSpillFront) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr - 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x3000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    sys2::AddressSegment seg2;
    status = asManager0.querySegment((void*)((uintptr_t)mapping.vaddr + 0x4000 - 1), &seg2);
    ASSERT_EQ(status, OsStatusSuccess);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 1);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 1);
}

TEST_F(AddressSpaceManagerTest, UnmapSpillBack) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr + 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x5000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg2;
    status = asManager0.querySegment((void*)((uintptr_t)mapping.vaddr + 0x4000 - 1), &seg2);
    ASSERT_EQ(status, OsStatusNotFound);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 1);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 1);
}

TEST_F(AddressSpaceManagerTest, UnmapMany) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.range, mappings[i].physicalRange());
        ASSERT_EQ(seg0.virtualRange(), mappings[i].virtualRange());
    }

    std::sort(mappings.begin(), mappings.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.vaddr < rhs.vaddr;
    });

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, mappings.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * mappings.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, range.size() - x64::kPageSize * 4 * mappings.size());

    km::VirtualRange all { mappings.front().vaddr, mappings.back().virtualRange().back };
    status = asManager0.unmap(&memory, all);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg1;
        status = asManager0.querySegment(mappings[i].vaddr, &seg1);
        ASSERT_EQ(status, OsStatusNotFound);
    }

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 0);
    ASSERT_EQ(stats1.heapStats.usedMemory, 0);
    ASSERT_EQ(stats1.heapStats.freeMemory, range.size());
}

TEST_F(AddressSpaceManagerTest, UnmapManyInner) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.range, mappings[i].physicalRange());
        ASSERT_EQ(seg0.virtualRange(), mappings[i].virtualRange());
    }

    std::sort(mappings.begin(), mappings.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.vaddr < rhs.vaddr;
    });

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, mappings.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * mappings.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, range.size() - x64::kPageSize * 4 * mappings.size());

    km::VirtualRange all { (void*)((uintptr_t)mappings.front().vaddr + 0x1000), (void*)((uintptr_t)mappings.back().virtualRange().back - 0x1000) };
    status = asManager0.unmap(&memory, all);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 1; i < mappings.size(); ++i) {
        sys2::AddressSegment seg1;
        status = asManager0.querySegment(mappings[i].vaddr, &seg1);
        ASSERT_EQ(status, OsStatusNotFound) << "i: " << i << " " << mappings[i].vaddr;
    }

    sys2::AddressSegment seg2;
    status = asManager0.querySegment(mappings.front().vaddr, &seg2);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg3;
    status = asManager0.querySegment((void*)((uintptr_t)mappings.back().virtualRange().back - 1), &seg3);
    ASSERT_EQ(status, OsStatusSuccess);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 2);
    ASSERT_EQ(stats1.heapStats.usedMemory, x64::kPageSize * 2);
    ASSERT_EQ(stats1.heapStats.freeMemory, range.size() - x64::kPageSize * 2);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 2);
}

TEST_F(AddressSpaceManagerTest, UnmapManySpillFront) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.range, mappings[i].physicalRange());
        ASSERT_EQ(seg0.virtualRange(), mappings[i].virtualRange());
    }

    std::sort(mappings.begin(), mappings.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.vaddr < rhs.vaddr;
    });

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, mappings.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * mappings.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, range.size() - x64::kPageSize * 4 * mappings.size());

    km::VirtualRange all { (void*)((uintptr_t)mappings.front().vaddr - 0x1000), (void*)((uintptr_t)mappings.back().virtualRange().back) };
    status = asManager0.unmap(&memory, all);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg1;
        status = asManager0.querySegment(mappings[i].vaddr, &seg1);
        ASSERT_EQ(status, OsStatusNotFound) << "i: " << i << " " << mappings[i].vaddr;
    }

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 0);
    ASSERT_EQ(stats1.heapStats.usedMemory, 0);
    ASSERT_EQ(stats1.heapStats.freeMemory, range.size());

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapManySpillBack) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.range, mappings[i].physicalRange());
        ASSERT_EQ(seg0.virtualRange(), mappings[i].virtualRange());
    }

    std::sort(mappings.begin(), mappings.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.vaddr < rhs.vaddr;
    });

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, mappings.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * mappings.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, range.size() - x64::kPageSize * 4 * mappings.size());

    km::VirtualRange all { (void*)((uintptr_t)mappings.front().vaddr), (void*)((uintptr_t)mappings.back().virtualRange().back + 0x1000) };
    status = asManager0.unmap(&memory, all);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg1;
        status = asManager0.querySegment(mappings[i].vaddr, &seg1);
        ASSERT_EQ(status, OsStatusNotFound) << "i: " << i << " " << mappings[i].vaddr;
    }

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 0);
    ASSERT_EQ(stats1.heapStats.usedMemory, 0);
    ASSERT_EQ(stats1.heapStats.freeMemory, range.size());

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapManySpillBack2) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys2::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.range, mappings[i].physicalRange());
        ASSERT_EQ(seg0.virtualRange(), mappings[i].virtualRange());
    }

    std::sort(mappings.begin(), mappings.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.vaddr < rhs.vaddr;
    });

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, mappings.size());
    ASSERT_EQ(stats0.heapStats.usedMemory, x64::kPageSize * 4 * mappings.size());
    ASSERT_EQ(stats0.heapStats.freeMemory, range.size() - x64::kPageSize * 4 * mappings.size());

    km::VirtualRange all { (void*)((uintptr_t)mappings.front().vaddr + 0x1000), (void*)((uintptr_t)mappings.back().virtualRange().back + 0x1000) };
    status = asManager0.unmap(&memory, all);
    ASSERT_EQ(status, OsStatusSuccess);

    asManager0.dump();

    for (size_t i = 1; i < mappings.size(); ++i) {
        sys2::AddressSegment seg1;
        status = asManager0.querySegment(mappings[i].vaddr, &seg1);
        ASSERT_EQ(status, OsStatusNotFound) << "i: " << i << " " << mappings[i].vaddr;
    }

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 1);
    ASSERT_EQ(stats1.heapStats.usedMemory, 0x1000);
    ASSERT_EQ(stats1.heapStats.freeMemory, range.size() - 0x1000);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 1);
}

TEST_F(AddressSpaceManagerTest, MapRemote) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();
    km::AddressMapping mapping0;

    status = sys2::MemoryManager::create(range, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = sys2::AddressSpaceManager::create(&pager, getPteMapping1(), km::PageFlags::eUserAll, vmem, &asManager1);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x1000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping0);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::AddressSegment seg0;
    status = asManager0.querySegment(mapping0.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.range, mapping0.physicalRange());
    ASSERT_EQ(seg0.virtualRange(), mapping0.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange range1;
    status = asManager1.map(&memory, &asManager0, mapping0.virtualRange(), km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &range1);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(range1.front, range1.back) << "Range " << std::string_view(km::format(range1));

    auto stats1 = asManager1.stats();
    ASSERT_EQ(stats1.segments, 1);

    sys2::AddressSegment seg1;
    status = asManager1.querySegment(range1.front, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg1.range, mapping0.physicalRange());
    ASSERT_EQ(seg1.virtualRange(), range1);
}
