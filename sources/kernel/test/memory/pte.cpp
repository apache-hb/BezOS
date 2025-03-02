#include <gtest/gtest.h>
#include <memory_resource>
#include <random>
#include <thread>

#include "memory/pte.hpp"
#include "setup.hpp"

static constexpr size_t kSize = 0x10000;

auto GetAllocatorMemory() {
    auto deleter = [](uint8_t *ptr) {
        :: operator delete[] (ptr, std::align_val_t(16));
    };

    return std::unique_ptr<uint8_t[], decltype(deleter)>{
        new (std::align_val_t(16)) uint8_t[kSize],
        deleter
    };
}

using TestMemory = decltype(GetAllocatorMemory());

struct BufferAllocator final : public mem::IAllocator {
    std::pmr::monotonic_buffer_resource mResource;

public:
    BufferAllocator(uint8_t *memory, size_t size)
        : mResource(memory, size)
    { }

    void *allocateAligned(size_t size, size_t align) override {
        return mResource.allocate(size, align);
    }

    void deallocate(void *ptr, size_t size) override {
        mResource.deallocate(ptr, size);
    }
};

class PageTableTest : public testing::Test {
public:
    void SetUp() override {
    }

    TestMemory memory = GetAllocatorMemory();
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };

    km::PageTables ptes(km::PageFlags flags) const {
        km::AddressMapping mapping { memory.get(), (uintptr_t)memory.get(), kSize };
        return km::PageTables { &pm, mapping, flags };
    }
};

TEST_F(PageTableTest, GetBackingAddress) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);
    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MapPage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MapPageOffset) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 123));

    ASSERT_EQ(paddr.address + 123, addr.address);

    km::PageSize size = pt.getPageSize((void*)((uintptr_t)vaddr + 123));
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MapSmallRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 64;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.map(range, vaddr, km::PageFlags::eAll);

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = pt.getBackingAddress((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(paddr.address + (i * x64::kPageSize), addr.address);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eRegular, size);
    }
}

TEST_F(PageTableTest, MapLargePage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kLargePageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eLarge, size);
}

TEST_F(PageTableTest, MapLargePageOffset) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kLargePageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 12345));

    ASSERT_EQ(paddr.address + 12345, addr.address);

    km::PageSize size = pt.getPageSize((void*)((uintptr_t)vaddr + 12345));
    ASSERT_EQ(km::PageSize::eLarge, size);
}

#if 0
TEST_F(PageTableTest, MapHugePage) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kHugePageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress(vaddr);

    ASSERT_EQ(paddr.address, addr.address);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eHuge, size);
}

TEST_F(PageTableTest, MapHugePageOffset) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kHugePageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PhysicalAddress addr = pt.getBackingAddress((void*)((uintptr_t)vaddr + 1234567));

    ASSERT_EQ(paddr.address + 1234567, addr.address);

    km::PageSize size = pt.getPageSize((void*)((uintptr_t)vaddr + 1234567));
    ASSERT_EQ(km::PageSize::eHuge, size);
}
#endif

TEST_F(PageTableTest, MapRangeLarge) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    ASSERT_EQ(paddr.address % x64::kLargePageSize, 0);

    size_t pages = 1024;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.map(range, vaddr, km::PageFlags::eAll);

    for (size_t i = 0; i < pages; i++) {
        km::PhysicalAddress addr = pt.getBackingAddress((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(paddr.address + (i * x64::kPageSize), addr.address);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eLarge, size);
    }
}

TEST_F(PageTableTest, MemoryFlags) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    km::AddressMapping mapping { vaddr, paddr, x64::kPageSize };

    pt.map(mapping, km::PageFlags::eAll);

    km::PageFlags flags = pt.getMemoryFlags(vaddr);

    ASSERT_EQ(km::PageFlags::eAll, flags);

    km::PageSize size = pt.getPageSize(vaddr);
    ASSERT_EQ(km::PageSize::eRegular, size);
}

TEST_F(PageTableTest, MemoryFlagsOnRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    size_t pages = 64;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.map(range, vaddr, km::PageFlags::eAll);

    for (size_t i = 0; i < pages; i++) {
        km::PageFlags flags = pt.getMemoryFlags((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageFlags::eAll, flags);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eRegular, size);
    }
}

TEST_F(PageTableTest, MemoryFlagsOnLargeRange) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;
    km::PhysicalAddress paddr = 0x1000000;
    ASSERT_EQ(paddr.address % x64::kLargePageSize, 0);

    size_t pages = 1024;
    km::MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    pt.map(range, vaddr, km::PageFlags::eCode);

    for (size_t i = 0; i < pages; i++) {
        km::PageFlags flags = pt.getMemoryFlags((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageFlags::eCode, flags);

        km::PageSize size = pt.getPageSize((char*)vaddr + (i * x64::kPageSize));
        ASSERT_EQ(km::PageSize::eLarge, size);
    }
}

TEST_F(PageTableTest, MemoryFlagsOnNullMapping) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;

    km::PageFlags flags = pt.getMemoryFlags(vaddr);

    ASSERT_EQ(km::PageFlags::eNone, flags);
}

TEST_F(PageTableTest, PageSizeOnNullMapping) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    const void *vaddr = (void*)0xFFFF800000000000;

    km::PageSize size = pt.getPageSize(vaddr);

    ASSERT_EQ(km::PageSize::eNone, size);
}

TEST_F(PageTableTest, MemoryFlagsKernelMapping) {
    uintptr_t kernel = 0x0000000007CC5000;
    uintptr_t hhdm = 0xFFFF800000000000;

    km::AddressMapping text = {
        .vaddr = (void*)0xFFFFFFFF80001000,
        .paddr = (0xFFFFFFFF80001000 - hhdm + kernel),
        .size = sm::roundup(0xFFFFFFFF8004101E - 0xFFFFFFFF80001000, x64::kPageSize),
    };

    km::AddressMapping rodata = {
        .vaddr = (void*)0xFFFFFFFF80042000,
        .paddr = (0xFFFFFFFF80042000 - hhdm + kernel),
        .size = sm::roundup(0xFFFFFFFF80047B88 - 0xFFFFFFFF80042000, x64::kPageSize),
    };

    km::AddressMapping data = {
        .vaddr = (void*)0xFFFFFFFF80048000,
        .paddr = (0xFFFFFFFF80048000 - hhdm + kernel),
        .size = sm::roundup(0xFFFFFFFF80049E40 - 0xFFFFFFFF80048000, x64::kPageSize),
    };

    km::PageTables pt = ptes(km::PageFlags::eAll);

    pt.map(text, km::PageFlags::eCode);
    pt.map(rodata, km::PageFlags::eRead);
    pt.map(data, km::PageFlags::eData);

    // ensure that all memory is mapped
    for (uintptr_t i = (uintptr_t)text.vaddr; i < (uintptr_t)text.vaddr + text.size; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eCode, flags);

        km::PhysicalAddress addr = pt.getBackingAddress((void*)i);
        ASSERT_EQ(text.paddr + (i - (uintptr_t)text.vaddr), addr.address);
    }

    for (uintptr_t i = (uintptr_t)rodata.vaddr; i < (uintptr_t)rodata.vaddr + rodata.size; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eRead, flags);

        km::PhysicalAddress addr = pt.getBackingAddress((void*)i);
        ASSERT_EQ(rodata.paddr + (i - (uintptr_t)rodata.vaddr), addr.address);
    }

    for (uintptr_t i = (uintptr_t)data.vaddr; i < (uintptr_t)data.vaddr + data.size; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eData, flags);

        km::PhysicalAddress addr = pt.getBackingAddress((void*)i);
        ASSERT_EQ(data.paddr + (i - (uintptr_t)data.vaddr), addr.address);
    }

    // ensure that nothing else was mapped
    // scan 1mb above the kernel
    for (uintptr_t i = kernel - 0x100000; i < kernel + 0x100000; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eNone, flags);
    }

    // scan 1mb below the kernel
    for (uintptr_t i = hhdm - 0x100000; i < hhdm + 0x100000; i += x64::kPageSize) {
        km::PageFlags flags = pt.getMemoryFlags((void*)i);
        ASSERT_EQ(km::PageFlags::eNone, flags);
    }
}

TEST(MemoryRoundTest, RoundUp) {
    ASSERT_EQ(sm::roundup(0x1000, 0x1000), 0x1000);
    ASSERT_EQ(sm::roundup(0x1001, 0x1000), 0x2000);
    ASSERT_EQ(sm::roundup(0x1FFF, 0x1000), 0x2000);
    ASSERT_EQ(sm::roundup(0x2000, 0x1000), 0x2000);
    ASSERT_EQ(sm::roundup(0, 0x1000), 0);
}

TEST(MemoryRoundTest, RoundDown) {
    ASSERT_EQ(sm::rounddown(0x1000, 0x1000), 0x1000);
    ASSERT_EQ(sm::rounddown(0x1001, 0x1000), 0x1000);
    ASSERT_EQ(sm::rounddown(0x1FFF, 0x1000), 0x1000);
    ASSERT_EQ(sm::rounddown(0x2000, 0x1000), 0x2000);
    ASSERT_EQ(sm::rounddown(0, 0x1000), 0);
}

TEST_F(PageTableTest, ThreadSafe) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    std::vector<std::jthread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&pt, i] {
            std::mt19937 gen { size_t(i) };
            for (int i = 0; i < 1000; i++) {
                auto paddr = gen();
                auto vaddr = gen();

                // align to 4k
                paddr &= ~0xFFF;
                vaddr &= ~0xFFF;

                const void *ptr = (void*)vaddr;
                km::AddressMapping mapping { ptr, paddr, x64::kPageSize };

                OsStatus status = pt.map(mapping, km::PageFlags::eAll);

                km::PhysicalAddress addr = pt.getBackingAddress(ptr);

                if (status == OsStatusSuccess) {
                    ASSERT_EQ(paddr, addr.address);
                } else {
                    ASSERT_EQ(KM_INVALID_MEMORY, addr.address);
                }

                km::PageSize size = pt.getPageSize(ptr);

                if (status == OsStatusSuccess) {
                    ASSERT_NE(km::PageSize::eNone, size);
                } else {
                    ASSERT_EQ(km::PageSize::eNone, size);
                }

                if (status == OsStatusSuccess) {
                    ASSERT_NE(km::PageFlags::eNone, pt.getMemoryFlags(ptr));
                } else {
                    ASSERT_EQ(km::PageFlags::eNone, pt.getMemoryFlags(ptr));
                }
            }
        });
    }

    threads.clear();
}

TEST_F(PageTableTest, AddressMapping) {
    km::PageTables pt = ptes(km::PageFlags::eAll);

    km::AddressMapping fb = {
        .vaddr = (void*)0xFFFFC00001000000,
        .paddr = 0x00000000FD000000,
        .size = 0x00000000FD3E8000 - 0x00000000FD000000,
    };

    pt.map(fb, km::PageFlags::eData, km::MemoryType::eWriteCombine);

    km::PageFlags front = pt.getMemoryFlags(fb.vaddr);
    ASSERT_EQ(km::PageFlags::eData, front);

    km::PageFlags back = pt.getMemoryFlags((char*)fb.vaddr + fb.size - 1);
    ASSERT_EQ(km::PageFlags::eData, back);
}
