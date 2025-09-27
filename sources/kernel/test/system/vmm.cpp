#include <gtest/gtest.h>

#include "system/vmm.hpp"
#include "memory/page_allocator.hpp"
#include "setup.hpp"
#include "system/pmm.hpp"

static constexpr km::MemoryRange kTestRange { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };

class AddressSpaceManagerTest : public testing::Test {
public:
    void SetUp() override {
        OsStatus status;

        km::VirtualRange vmem = kTestRange.cast<const void*>();
        pteMemory0.reset(new x64::page[1024]);
        pteMemory1.reset(new x64::page[1024]);

        std::vector<boot::MemoryRegion> memmap = {
            { .type = boot::MemoryRegion::eUsable, .range = kTestRange }
        };

        status = km::PageAllocator::create(memmap, &pmm);
        ASSERT_EQ(status, OsStatusSuccess);

        status = sys::MemoryManager::create(&pmm, &memory);
        ASSERT_EQ(status, OsStatusSuccess);

        status = sys::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
        ASSERT_EQ(status, OsStatusSuccess);

        status = sys::AddressSpaceManager::create(&pager, getPteMapping1(), km::PageFlags::eUserAll, vmem, &asManager1);
        ASSERT_EQ(status, OsStatusSuccess);

        AssertStats0(0, 0);
        AssertStats1(0, 0);
        AssertMemory(0, 0);
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

    void AssertStats(sys::AddressSpaceManager& asManager, size_t segments, size_t memory) {
        auto stats = asManager.stats();
        ASSERT_EQ(stats.segments, segments);
        ASSERT_EQ(stats.heapStats.usedMemory, memory);
        ASSERT_EQ(stats.heapStats.freeMemory, kTestRange.size() - memory);
    }

    void AssertStats0(size_t segments, size_t memory) {
        AssertStats(asManager0, segments, memory);
    }

    void AssertStats1(size_t segments, size_t memory) {
        AssertStats(asManager1, segments, memory);
    }

    void AssertMemory(size_t segments, size_t used) {
        auto stats = memory.stats();
        ASSERT_EQ(stats.segments, segments);
        ASSERT_EQ(stats.heapStats.usedMemory, used);
        ASSERT_EQ(stats.heapStats.freeMemory, kTestRange.size() - used);
    }

    void AssertVirtualFound0(const km::VirtualRange& range, const km::MemoryRangeEx& memory) {
        AssertVirtualFound0(range.cast<sm::VirtualAddress>(), memory);
    }

    void AssertVirtualFound0(const km::VirtualRangeEx& range, const km::MemoryRangeEx& memory) {
        AssertVirtualFound0(range.front, range, memory);
        AssertVirtualFound0(range.back - 1, range, memory);
        AssertVirtualFound0((range.front.address + range.back.address) / 2, range, memory);
    }

    void AssertVirtualFound0(sm::VirtualAddress address, const km::VirtualRange& range, const km::MemoryRangeEx& memory) {
        AssertVirtualFound0(address, range.cast<sm::VirtualAddress>(), memory);
    }

    void AssertVirtualFound0(sm::VirtualAddress address, const km::VirtualRangeEx& range, const km::MemoryRangeEx& memory) {
        AssertVirtualFound(asManager0, address, range, memory);
    }

    void AssertVirtualFound(sys::AddressSpaceManager& asman, sm::VirtualAddress address, const km::VirtualRangeEx& range, const km::MemoryRangeEx& memory) {
        sys::AddressSegment segment;
        OsStatus status = asman.querySegment(address, &segment);
        ASSERT_EQ(status, OsStatusSuccess)
            << "Failed to find segment for address: "
            << std::string_view(km::format(address))
            << " in range: "
            << std::string_view(km::format(range));

        ASSERT_EQ(segment.range(), range.cast<const void*>())
            << "Range mismatch: "
            << std::string_view(km::format(segment.range()))
            << " != " << std::string_view(km::format(range));

        ASSERT_EQ(segment.getBackingMemory(), memory.cast<km::PhysicalAddress>())
            << "Backing memory mismatch: "
            << std::string_view(km::format(segment.getBackingMemory()))
            << " != " << std::string_view(km::format(memory));
    }

    void AssertVirtualNotFound0(sm::VirtualAddress address) {
        AssertVirtualNotFound(asManager0, address);
    }

    void AssertVirtualNotFound(sys::AddressSpaceManager& asman, sm::VirtualAddress address) {
        sys::AddressSegment segment;
        OsStatus status = asman.querySegment(address, &segment);
        ASSERT_EQ(status, OsStatusNotFound)
            << "Found segment for address: "
            << std::string_view(km::format(address));
    }

    std::vector<km::AddressMapping> mapMany(size_t count, size_t size) {
        std::vector<km::AddressMapping> mappings;
        mappings.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            km::AddressMapping mapping;
            OsStatus status = asManager0.map(&memory, size, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
            EXPECT_EQ(status, OsStatusSuccess);
            mappings.push_back(mapping);
        }

        AssertStats0(count, size * count);
        AssertMemory(count, size * count);
        std::sort(mappings.begin(), mappings.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.vaddr < rhs.vaddr;
        });

        return mappings;
    }

    std::unique_ptr<x64::page[]> pteMemory0;
    std::unique_ptr<x64::page[]> pteMemory1;
    km::PageAllocator pmm;
    sys::MemoryManager memory;
    sys::AddressSpaceManager asManager0;
    sys::AddressSpaceManager asManager1;
    km::PageBuilder pager { 48, 48, km::GetDefaultPatLayout() };
};

TEST(AddressSpaceManagerConstructTest, Construct) {
    OsStatus status = OsStatusSuccess;
    km::MemoryRange range = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };
    km::VirtualRange vmem = range.cast<const void*>();

    std::vector<boot::MemoryRegion> memmap = {
        { .type = boot::MemoryRegion::eUsable, .range = kTestRange }
    };

    km::PageAllocator pmm;
    status = km::PageAllocator::create(memmap, &pmm);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::MemoryManager memory;
    status = sys::MemoryManager::create(&pmm, &memory);
    ASSERT_EQ(status, OsStatusSuccess);

    std::unique_ptr<x64::page[]> pteMemory0;
    pteMemory0.reset(new x64::page[1024]);

    km::AddressMapping pteMapping0 {
        .vaddr = std::bit_cast<const void*>(pteMemory0.get()),
        .paddr = std::bit_cast<km::PhysicalAddress>(pteMemory0.get()),
        .size = 1024 * sizeof(x64::page),
    };

    sys::AddressSpaceManager asManager0;
    km::PageBuilder pager { 48, 48, km::GetDefaultPatLayout() };
    status = sys::AddressSpaceManager::create(&pager, pteMapping0, km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 0);
}

TEST_F(AddressSpaceManagerTest, Allocate) {
    OsStatus status = OsStatusSuccess;
    km::AddressMapping mapping;

    status = asManager0.map(&memory, 0x1000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    AssertVirtualFound0(mapping.vaddr, mapping.virtualRange(), mapping.physicalRangeEx());

    AssertStats0(1, 0x1000);

    status = asManager0.unmap(&memory, mapping.virtualRange());
    ASSERT_EQ(status, OsStatusSuccess);

    AssertVirtualNotFound0(mapping.vaddr);

    AssertStats0(0, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapMiddle) {
    OsStatus status = OsStatusSuccess;
    km::AddressMapping mapping;

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    AssertVirtualFound0(mapping.vaddr, mapping.virtualRange(), mapping.physicalRangeEx());
    AssertStats0(1, mapping.size);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr + 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x3000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    AssertVirtualFound0(mapping.virtualRange().first(0x1000), mapping.physicalRangeEx().first(0x1000));
    AssertVirtualFound0(mapping.virtualRange().last(0x1000), mapping.physicalRangeEx().last(0x1000));
    AssertVirtualNotFound0(mapping.virtualRange().cast<sm::VirtualAddress>().front + 0x2000);

    AssertStats0(2, 0x2000);
    AssertMemory(2, 0x2000);
}

TEST_F(AddressSpaceManagerTest, UnmapExact) {
    OsStatus status = OsStatusSuccess;
    km::AddressMapping mapping;

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    AssertVirtualFound0(mapping.virtualRangeEx(), mapping.physicalRangeEx());

    AssertStats0(1, 0x4000);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr), (void*)((uintptr_t)mapping.vaddr + 0x4000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    AssertVirtualNotFound0(mapping.vaddr);
    AssertVirtualNotFound0(subrange.back);
    AssertVirtualNotFound0((void*)((uintptr_t)mapping.vaddr + 0x2000));

    AssertStats0(0, 0);
    AssertStats1(0, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapFront) {
    OsStatus status = OsStatusSuccess;
    km::AddressMapping mapping;

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.getBackingMemory(), mapping.physicalRange());
    ASSERT_EQ(seg0.range(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { mapping.vaddr, (void*)((uintptr_t)mapping.vaddr + 0x2000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg1;
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

    status = sys::AddressSpaceManager::create(&pager, getPteMapping0(), km::PageFlags::eUserAll, vmem, &asManager0);
    ASSERT_EQ(status, OsStatusSuccess);

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.getBackingMemory(), mapping.physicalRange());
    ASSERT_EQ(seg0.range(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr + 0x2000), (void*)((uintptr_t)mapping.vaddr + 0x4000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg1;
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
    km::AddressMapping mapping;

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.getBackingMemory(), mapping.physicalRange());
    ASSERT_EQ(seg0.range(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr - 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x5000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 0);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 0);
}

TEST_F(AddressSpaceManagerTest, UnmapSpillFront) {
    OsStatus status = OsStatusSuccess;
    km::AddressMapping mapping;

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.getBackingMemory(), mapping.physicalRange());
    ASSERT_EQ(seg0.range(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr - 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x3000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusNotFound);

    sys::AddressSegment seg2;
    status = asManager0.querySegment((void*)((uintptr_t)mapping.vaddr + 0x4000 - 1), &seg2);
    ASSERT_EQ(status, OsStatusSuccess);

    auto stats1 = asManager0.stats();
    ASSERT_EQ(stats1.segments, 1);

    auto memstats = memory.stats();
    ASSERT_EQ(memstats.segments, 1);
}

TEST_F(AddressSpaceManagerTest, UnmapSpillBack) {
    OsStatus status = OsStatusSuccess;
    km::AddressMapping mapping;

    status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg0;
    status = asManager0.querySegment(mapping.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.getBackingMemory(), mapping.physicalRange());
    ASSERT_EQ(seg0.range(), mapping.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange subrange { (void*)((uintptr_t)mapping.vaddr + 0x1000), (void*)((uintptr_t)mapping.vaddr + 0x5000) };
    status = asManager0.unmap(&memory, subrange);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg1;
    status = asManager0.querySegment(mapping.vaddr, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg2;
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

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.getBackingMemory(), mappings[i].physicalRange());
        ASSERT_EQ(seg0.range(), mappings[i].virtualRange());
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
        sys::AddressSegment seg1;
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

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.getBackingMemory(), mappings[i].physicalRange());
        ASSERT_EQ(seg0.range(), mappings[i].virtualRange());
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
        sys::AddressSegment seg1;
        status = asManager0.querySegment(mappings[i].vaddr, &seg1);
        ASSERT_EQ(status, OsStatusNotFound) << "i: " << i << " " << mappings[i].vaddr;
    }

    sys::AddressSegment seg2;
    status = asManager0.querySegment(mappings.front().vaddr, &seg2);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg3;
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

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.getBackingMemory(), mappings[i].physicalRange());
        ASSERT_EQ(seg0.range(), mappings[i].virtualRange());
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
        sys::AddressSegment seg1;
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

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.getBackingMemory(), mappings[i].physicalRange());
        ASSERT_EQ(seg0.range(), mappings[i].virtualRange());
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
        sys::AddressSegment seg1;
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

    std::array<km::AddressMapping, 4> mappings;
    for (size_t i = 0; i < mappings.size(); ++i) {
        status = asManager0.map(&memory, 0x4000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mappings[i]);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    for (size_t i = 0; i < mappings.size(); ++i) {
        sys::AddressSegment seg0;
        status = asManager0.querySegment(mappings[i].vaddr, &seg0);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(seg0.getBackingMemory(), mappings[i].physicalRange());
        ASSERT_EQ(seg0.range(), mappings[i].virtualRange());
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

    for (size_t i = 1; i < mappings.size(); ++i) {
        sys::AddressSegment seg1;
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
    km::AddressMapping mapping0;

    status = asManager0.map(&memory, 0x1000, 0x1000, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping0);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::AddressSegment seg0;
    status = asManager0.querySegment(mapping0.vaddr, &seg0);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg0.getBackingMemory(), mapping0.physicalRange());
    ASSERT_EQ(seg0.range(), mapping0.virtualRange());

    auto stats0 = asManager0.stats();
    ASSERT_EQ(stats0.segments, 1);

    km::VirtualRange range1;
    status = asManager1.map(&memory, &asManager0, mapping0.virtualRange(), km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &range1);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(range1.front, range1.back) << "Range " << std::string_view(km::format(range1));

    auto stats1 = asManager1.stats();
    ASSERT_EQ(stats1.segments, 1);

    sys::AddressSegment seg1;
    status = asManager1.querySegment(range1.front, &seg1);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(seg1.getBackingMemory(), mapping0.physicalRange());
    ASSERT_EQ(seg1.range(), range1);
}

class AddressSpaceMapManyTest : public AddressSpaceManagerTest {
public:
    void SetUp() override {
        AddressSpaceManagerTest::SetUp();
        mappings = mapMany(4, 0x4000);
    }

    std::vector<km::AddressMapping> mappings;
};

TEST_F(AddressSpaceMapManyTest, UnmapPartial) {

}
