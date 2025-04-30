#include <gtest/gtest.h>

#include "memory/page_allocator.hpp"

using km::PageAllocator;
using boot::MemoryRegion;
using km::MemoryRange;

class PageAllocatorTest : public testing::Test {
public:
    // | 0000  | 0x0000000000000000 | 1mb                | Reserved
    // | 0001  | 0x0000000000001000 | 328kb              | Bootloader reclaimable
    // | 0002  | 0x0000000000053000 | 304kb              | Usable
    // | 0003  | 0x000000000009FC00 | 1kb                | Reserved
    // | 0004  | 0x00000000000F0000 | 64kb               | Reserved
    // | 0005  | 0x0000000000100000 | 64kb               | Bootloader reclaimable
    // | 0006  | 0x0000000000110000 | 1mb                | Kernel runtime data
    // | 0007  | 0x0000000000210000 | 16mb               | Kernel runtime data
    // | 0008  | 0x0000000001210000 | 83mb+404kb         | Usable
    // | 0009  | 0x0000000006575000 | 5mb+172kb          | Kernel and modules
    // | 0010  | 0x0000000006AA0000 | 4kb                | Bootloader reclaimable
    // | 0011  | 0x0000000006AA1000 | 4kb                | Usable
    // | 0012  | 0x0000000006AA2000 | 4kb                | Bootloader reclaimable
    // | 0013  | 0x0000000006AA3000 | 4kb                | Usable
    // | 0014  | 0x0000000006AA4000 | 44kb               | Bootloader reclaimable
    // | 0015  | 0x0000000006AAF000 | 4mb+764kb          | Kernel and modules
    // | 0016  | 0x0000000006F6E000 | 15mb+688kb         | Bootloader reclaimable
    // | 0017  | 0x0000000007F1A000 | 88kb               | Usable
    // | 0018  | 0x0000000007F30000 | 700kb              | Bootloader reclaimable
    // | 0019  | 0x0000000007FDF000 | 132kb              | Reserved
    // | 0020  | 0x00000000B0000000 | 256mb              | Reserved
    // | 0021  | 0x00000000FD000000 | 3mb+928kb          | Framebuffer
    // | 0022  | 0x00000000FD000000 | 3mb+928kb          | Framebuffer
    // | 0023  | 0x00000000FED1C000 | 16kb               | Reserved
    // | 0024  | 0x00000000FFFC0000 | 256kb              | Reserved
    // | 0025  | 0x000000FD00000000 | 12gb               | Reserved
    void SetUp() override {
        std::vector<MemoryRegion> regions {
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

        OsStatus status = PageAllocator::create(regions, &allocator);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    PageAllocator allocator;
};

TEST_F(PageAllocatorTest, Allocate4k) {
    auto it = allocator.pageAlloc(1);
    ASSERT_TRUE(it.isValid());

    auto range = it.range();
    EXPECT_EQ(range.size(), x64::kPageSize);
    EXPECT_GE(range.front, sm::megabytes(1).bytes());
}
