#include <gtest/gtest.h>

#include <cstdlib>

#include <mm_malloc.h>
#include <stdint.h>

#include "memory/virtual_allocator.hpp"
#include "concurrentqueue.h"

#include <thread>
#include <unordered_set>

TEST(VmemTest, Construction) {
    uintptr_t front = -(1ull << 47);
    uintptr_t back = -(1ull << 46);
    km::VirtualAllocator vmem { { (void*)front, (void*)back } };
}

TEST(VmemTest, Allocate) {
    uintptr_t front = -(1ull << 47);
    uintptr_t back = -(1ull << 46);

    ASSERT_EQ(front, 0xFFFF800000000000);
    ASSERT_EQ(back, 0xFFFFC00000000000);

    km::VirtualRange range { (void*)front, (void*)back };
    ASSERT_GT(range.size(), 0x10000);
    ASSERT_TRUE(range.isValid());

    km::VirtualAllocator vmem { range };

    void *addr = vmem.alloc4k(1);
    ASSERT_EQ(addr, (void*)front);

    void *addr2 = vmem.alloc4k(1);
    ASSERT_NE(addr2, addr);
    ASSERT_NE(addr2, nullptr);
}

TEST(VmemTest, AllocateLarge) {
    uintptr_t front = -(1ull << 47);
    uintptr_t back = -(1ull << 46);

    ASSERT_EQ(front, 0xFFFF800000000000);
    ASSERT_EQ(back, 0xFFFFC00000000000);

    km::VirtualRange range { (void*)front, (void*)back };
    ASSERT_GT(range.size(), 0x10000);
    ASSERT_TRUE(range.isValid());

    km::VirtualAllocator vmem { range };

    void *addr = vmem.alloc2m(1);
    ASSERT_EQ(addr, (void*)front);

    void *addr2 = vmem.alloc2m(1);
    ASSERT_NE(addr2, addr);
    ASSERT_NE(addr2, nullptr);
}

TEST(RoundupTest, RoundUp) {
    ASSERT_EQ(0x1000, sm::roundup(0x1000, 1));
    ASSERT_EQ(0xffff800000000000ull, sm::roundup(0xffff800000000000ull, 0x1000ull));
}

TEST(VmemTest, ThreadSafe) {
    km::VirtualRange range { (void*)0x1000, (void*)0x100000000000 };
    km::VirtualAllocator allocator { range };

    moodycamel::ConcurrentQueue<km::VirtualRange> ranges;
    std::vector<std::jthread> threads;

    for (int i = 0; i < 15; i++) {
        threads.emplace_back([&] {
            for (int j = 0; j < 300; j++) {
                void *addr = (j % 2 == 0) ? allocator.alloc4k(1) : allocator.userAlloc4k(1);
                if (addr == KM_INVALID_ADDRESS) continue;

                ranges.enqueue(km::VirtualRange { addr, (void*)((uintptr_t)addr + 0x1000) });
            }
        });
    }

    threads.clear();

    // ensure that all ranges are unique
    std::vector<km::VirtualRange> all;
    km::VirtualRange r;
    while (ranges.try_dequeue(r)) {
        all.push_back(r);
    }

    std::unordered_set<const void*> seen;
    for (const auto& r : all) {
        if (r.isEmpty()) continue;

        ASSERT_FALSE(seen.contains(r.front)) << "Duplicate range: " << r.front << " - " << r.back;
        seen.insert(r.front);
    }

    // make sure that ranges dont add up to more than the total range
    size_t total = 0;
    for (const auto& each : all) {
        total += each.size();
    }

    ASSERT_LE(total, range.size());
}
