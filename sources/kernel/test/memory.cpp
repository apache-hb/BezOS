#include <gtest/gtest.h>
#include <thread>

#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"
#include "memory/allocator.hpp"
#include "test_memory.hpp"

// dont worry about it :)
#include "tlsf.c"

TEST(MemoryTest, Construct) {
    SystemMemoryTestBody body;
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    km::SystemMemory memory = body.make();
}

TEST(MemoryTest, MapKernelAddress) {
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    km::SystemMemory memory = body.makeNoAlloc();

    void *kptr = memory.map({ s0.front, s0.front + 0x1000 });
    ASSERT_NE(kptr, nullptr);
    ASSERT_TRUE(body.pm.isHigherHalf(kptr));
}

TEST(MemoryTest, ThreadSafe) {
    SystemMemoryTestBody body;
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    km::SystemMemory memory = body.make();
    std::atomic<size_t> allocs = 0;
    std::vector<std::jthread> threads;
    for (size_t i = 0; i < 10; i++) {
        threads.emplace_back([&] {
            for (size_t i = 0; i < 1000; i++) {
                km::MappingAllocation allocation;
                OsStatus status = memory.map(0x1000, km::PageFlags::eData, km::MemoryType::eWriteCombine, &allocation);
                if (status == OsStatusOutOfMemory) continue;
                ASSERT_EQ(status, OsStatusSuccess);
                status = memory.unmap(allocation);
                ASSERT_EQ(status, OsStatusSuccess);
                allocs++;
            }
        });
    }

    ASSERT_NE(allocs, 0) << "No allocations";

    threads.clear();

    ASSERT_TRUE(true) << "No crashes";
}

TEST(MemoryTest, SynchronizedAlloc) {
    void *ptr = aligned_alloc(x64::kPageSize, 0x1000000);
    mem::SynchronizedAllocator<mem::TlsfAllocator> alloc(ptr, 0x1000000);

    std::vector<std::jthread> threads;
    for (size_t i = 0; i < 10; i++) {
        threads.emplace_back([&alloc] {
            for (size_t i = 0; i < 1000; i++) {
                void *ptr = alloc.allocate(32);

                if (ptr != nullptr) {
                    alloc.deallocate(ptr, 32);
                }
            }
        });
    }

    threads.clear();

    ASSERT_TRUE(true) << "No crashes";

    free(ptr);
}
