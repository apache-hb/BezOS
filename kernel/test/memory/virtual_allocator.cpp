#include <gtest/gtest.h>

#include <cstdlib>

#include <mm_malloc.h>
#include <ostream>
#include <stdint.h>

#include "memory/virtual_allocator.hpp"

struct GlobalAllocator final : public mem::IAllocator {
    void *allocateAligned(size_t size, size_t align) override {
        std::cout << "Allocating " << size << " bytes with alignment " << align << std::endl;
        return _mm_malloc(size, align);
    }

    void deallocate(void *ptr, size_t _) override {
        free(ptr);
    }
};

static GlobalAllocator gAllocator;

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
