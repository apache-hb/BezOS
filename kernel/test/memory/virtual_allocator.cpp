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
    uintptr_t back = -(1ull << 48);
    uintptr_t front = -(1ull << 47);
    km::VirtualAllocator vmem { { (void*)front, (void*)back }, &gAllocator };
}

TEST(VmemTest, Allocate) {
    uintptr_t back = -(1ull << 48);
    uintptr_t front = -(1ull << 47);
    km::VirtualAllocator vmem { { (void*)front, (void*)back }, &gAllocator };

    void *addr = vmem.alloc4k(1);
    ASSERT_EQ(addr, (void*)front);

    void *addr2 = vmem.alloc4k(1);
    ASSERT_NE(addr2, addr);
}
