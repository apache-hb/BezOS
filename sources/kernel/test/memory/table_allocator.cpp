#include <gtest/gtest.h>
#include <random>

#include "memory/table_allocator.hpp"

using namespace km;

static constexpr size_t kSize = 0x10000;

auto GetAllocatorMemory(size_t size = kSize) {
    auto deleter = [](uint8_t *ptr) {
        :: operator delete[] (ptr, std::align_val_t(x64::kPageSize));
    };

    return std::unique_ptr<uint8_t[], decltype(deleter)>{
        new (std::align_val_t(x64::kPageSize)) uint8_t[size],
        deleter
    };
}

using TestMemory = decltype(GetAllocatorMemory());

static void IsValidPtr(const void *ptr) {
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % x64::kPageSize, 0);
}

TEST(TableAllocatorTest, Construct) {
    TestMemory memory = GetAllocatorMemory();
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };
}

TEST(TableAllocatorTest, Allocate) {
    TestMemory memory = GetAllocatorMemory();
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };

    void *ptr = allocator.allocate(1);
    IsValidPtr(ptr);

    allocator.deallocate(ptr, 1);
}

TEST(TableAllocatorTest, AllocateMany) {
    size_t count = 16;
    size_t size = x64::kPageSize * count;
    TestMemory memory = GetAllocatorMemory(size);
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };

    std::set<void*> ptrs;
    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1);
        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));

        ptrs.insert(ptr);
    }

    for (void *ptr : ptrs) {
        allocator.deallocate(ptr, 1);
    }

    ptrs.clear();

    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1);
        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));

        ptrs.insert(ptr);
    }

    for (void *ptr : ptrs) {
        allocator.deallocate(ptr, 1);
    }
}

TEST(TableAllocatorTest, AllocateOom) {
    size_t count = 16;
    size_t size = x64::kPageSize * count;
    TestMemory memory = GetAllocatorMemory(size);
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };

    std::set<void*> ptrs;
    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1);
        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));

        ptrs.insert(ptr);
    }

    for (size_t i = 0; i < 10; i++) {
        void *ptr = allocator.allocate(1);
        ASSERT_EQ(ptr, nullptr);
    }

    for (void *ptr : ptrs) {
        allocator.deallocate(ptr, 1);
    }
}

TEST(TableAllocatorTest, AllocateSized) {
    size_t count = 16;
    size_t size = x64::kPageSize * count;
    TestMemory memory = GetAllocatorMemory(size);
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };

    std::mt19937 gen { 0x1234 };
    std::uniform_int_distribution<size_t> dist { 1, 4 };

    std::map<void*, size_t> ptrs;
    for (size_t i = 0; i < count; i++) {
        size_t alloc = dist(gen);
        void *ptr = allocator.allocate(alloc);
        if (ptr == nullptr) continue;

        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));

        ptrs[ptr] = alloc;
    }

    for (auto& [ptr, size] : ptrs) {
        allocator.deallocate(ptr, size);
    }
}

TEST(TableAllocatorTest, PartialDeallocate) {
    size_t count = 16;
    size_t size = x64::kPageSize * count;
    TestMemory memory = GetAllocatorMemory(size);
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };

    void *ptr = allocator.allocate(4);
    IsValidPtr(ptr);

    void *middle = (void*)((uintptr_t)ptr + x64::kPageSize);
    allocator.deallocate(middle, 1);

    void *next = (void*)((uintptr_t)middle + x64::kPageSize);
    allocator.deallocate(next, 2);

    allocator.deallocate(ptr, 1);

    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1);
        IsValidPtr(ptr);

        allocator.deallocate(ptr, 1);
    }
}
