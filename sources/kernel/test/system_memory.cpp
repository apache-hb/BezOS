#include <gtest/gtest.h>

#include "memory.hpp"
#include "memory/allocator.hpp"
#include "setup.hpp"

using namespace km;
using namespace boot;

class SystemMemoryConstructTest : public testing::Test {
public:
    static constexpr VirtualRangeEx kTestVirtualRange = { sm::megabytes(1).bytes(), sm::gigabytes(4).bytes() };
    static const inline std::vector<MemoryRegion> regions {
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x0000000000000000zu, sm::megabytes(1).bytes())   },
        MemoryRegion { MemoryRegion::eUsable,                  MemoryRange::of(0x0000000000001000zu, sm::kilobytes(328).bytes()) },
        MemoryRegion { MemoryRegion::eUsable,                  MemoryRange::of(0x0000000000053000zu, sm::kilobytes(304).bytes()) },
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x000000000009FC00zu, sm::kilobytes(1).bytes())   },
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x00000000000F0000zu, sm::kilobytes(64).bytes())  },
        MemoryRegion { MemoryRegion::eBootloaderReclaimable,   MemoryRange::of(0x0000000000100000zu, sm::kilobytes(64).bytes())  },
        MemoryRegion { MemoryRegion::eKernel,                  MemoryRange::of(0x0000000000110000zu, sm::megabytes(1).bytes())   },
        MemoryRegion { MemoryRegion::eKernel,                  MemoryRange::of(0x0000000000210000zu, sm::megabytes(16).bytes())  },
        MemoryRegion { MemoryRegion::eUsable,                  MemoryRange::of(0x0000000001210000zu, sm::megabytes(83).bytes() + sm::kilobytes(404).bytes()) },
        MemoryRegion { MemoryRegion::eKernel,                  MemoryRange::of(0x0000000006575000zu, sm::megabytes(5).bytes() + sm::kilobytes(172).bytes()) },
        MemoryRegion { MemoryRegion::eBootloaderReclaimable,   MemoryRange::of(0x0000000006AA0000zu, sm::kilobytes(4).bytes())    },
        MemoryRegion { MemoryRegion::eUsable,                  MemoryRange::of(0x0000000006AA1000zu, sm::kilobytes(4).bytes())    },
        MemoryRegion { MemoryRegion::eBootloaderReclaimable,   MemoryRange::of(0x0000000006AA2000zu, sm::kilobytes(4).bytes())    },
        MemoryRegion { MemoryRegion::eUsable,                  MemoryRange::of(0x0000000006AA3000zu, sm::kilobytes(4).bytes())    },
        MemoryRegion { MemoryRegion::eBootloaderReclaimable,   MemoryRange::of(0x0000000006AA4000zu, sm::kilobytes(44).bytes())   },
        MemoryRegion { MemoryRegion::eKernel,                  MemoryRange::of(0x0000000006AAF000zu, sm::megabytes(4).bytes() + sm::kilobytes(764).bytes()) },
        MemoryRegion { MemoryRegion::eBootloaderReclaimable,   MemoryRange::of(0x0000000006F6E000zu, sm::megabytes(15).bytes() + sm::kilobytes(688).bytes()) },
        MemoryRegion { MemoryRegion::eUsable,                  MemoryRange::of(0x0000000007F1A000zu, sm::kilobytes(88).bytes())   },
        MemoryRegion { MemoryRegion::eBootloaderReclaimable,   MemoryRange::of(0x0000000007F30000zu, sm::kilobytes(700).bytes())  },
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x0000000007FDF000zu, sm::kilobytes(132).bytes())  },
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x00000000B0000000zu, sm::megabytes(256).bytes())  },
        MemoryRegion { MemoryRegion::eFrameBuffer,             MemoryRange::of(0x00000000FD000000zu, sm::megabytes(3).bytes() + sm::kilobytes(928).bytes()) },
        MemoryRegion { MemoryRegion::eFrameBuffer,             MemoryRange::of(0x00000000FD000000zu, sm::megabytes(3).bytes() + sm::kilobytes(928).bytes()) },
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x00000000FED1C000zu, sm::kilobytes(16).bytes())   },
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x00000000FFFC0000zu, sm::kilobytes(256).bytes())  },
        MemoryRegion { MemoryRegion::eReserved,                MemoryRange::of(0x000000FD00000000zu, sm::gigabytes(12).bytes())   },
    };

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

class SystemMemoryTest : public SystemMemoryConstructTest {
public:
    void SetUp() override {
        SystemMemoryConstructTest::SetUp();

        OsStatus status = km::SystemMemory::create(regions, kTestVirtualRange, pager, pteMemory, &memory);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    SystemMemory memory;
};

TEST_F(SystemMemoryConstructTest, Construct) {
    km::SystemMemory memory;
    OsStatus status = km::SystemMemory::create(regions, kTestVirtualRange, pager, pteMemory, &memory);
    ASSERT_EQ(status, OsStatusSuccess);
}

TEST_F(SystemMemoryTest, Allocate) {
    MappingAllocation allocation;
    OsStatus status = memory.map(x64::kPageSize * 4, PageFlags::eData, MemoryType::eWriteBack, &allocation);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(allocation.size(), x64::kPageSize * 4);

    status = memory.unmap(allocation);
    ASSERT_EQ(status, OsStatusSuccess);
}
