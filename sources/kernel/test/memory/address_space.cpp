#include <gtest/gtest.h>

#include "memory/address_space.hpp"
#include "memory/allocator.hpp"
#include "setup.hpp"

using namespace km;

class AddressSpaceConstructTest : public testing::Test {
public:
    static constexpr km::VirtualRangeEx kTestVirtualRange = { sm::megabytes(1).bytes(), sm::gigabytes(4).bytes() };
    void SetUp() override {
        pteStorage.reset(new x64::page[1024]);

        pteMemory = {
            .vaddr = std::bit_cast<void*>(pteStorage.get()),
            .paddr = std::bit_cast<PhysicalAddress>(pteStorage.get()),
            .size = sizeof(x64::page) * 1024,
        };
    }

    std::unique_ptr<x64::page[]> pteStorage;
    km::AddressMapping pteMemory;
    km::PageBuilder pager { 48, 48, GetDefaultPatLayout() };
};

class AddressSpaceTest : public AddressSpaceConstructTest {
public:
    static constexpr km::MemoryRangeEx kTestMemoryRange = { sm::gigabytes(1).bytes(), sm::gigabytes(4).bytes() };

    void SetUp() override {
        AddressSpaceConstructTest::SetUp();

        OsStatus status = km::AddressSpace::create(&pager, pteMemory, km::PageFlags::eAll, kTestVirtualRange, &space);
        ASSERT_EQ(status, OsStatusSuccess);

        status = km::TlsfHeap::create(kTestMemoryRange.cast<km::PhysicalAddress>(), &heap);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    AddressSpace space;
    TlsfHeap heap;
};

TEST_F(AddressSpaceConstructTest, Construct) {
    AddressSpace space;
    OsStatus status = km::AddressSpace::create(&pager, pteMemory, km::PageFlags::eAll, kTestVirtualRange, &space);
    ASSERT_EQ(status, OsStatusSuccess);
}

TEST_F(AddressSpaceTest, Map) {
    MemoryRangeEx range { 0x1000, 0x4000 };
    VmemAllocation allocation;
    OsStatus status = space.map(range, PageFlags::eData, MemoryType::eWriteBack, &allocation);
    ASSERT_EQ(status, OsStatusSuccess);
}

TEST_F(AddressSpaceTest, MapAllocation) {
    TlsfAllocation memory = heap.aligned_alloc(x64::kPageSize, 0x4000);
    ASSERT_TRUE(memory.isValid());

    MappingAllocation allocation;
    OsStatus status = space.map(memory, PageFlags::eData, MemoryType::eWriteBack, &allocation);
    ASSERT_EQ(status, OsStatusSuccess);
}

TEST_F(AddressSpaceTest, UnmapAllocation) {
    TlsfAllocation memory = heap.aligned_alloc(x64::kPageSize, 0x4000);
    ASSERT_TRUE(memory.isValid());

    MappingAllocation allocation;
    OsStatus status = space.map(memory, PageFlags::eData, MemoryType::eWriteBack, &allocation);
    ASSERT_EQ(status, OsStatusSuccess);

    status = space.unmap(allocation);
    ASSERT_EQ(status, OsStatusSuccess);
}
