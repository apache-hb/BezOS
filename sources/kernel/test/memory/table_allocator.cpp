#include <gtest/gtest.h>

#include <random>
#include <thread>

#include "memory/table_allocator.hpp"
#include "log.hpp"

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

TEST(TableAllocatorDetailTest, SwapBlocks) {
    TestMemory memory = GetAllocatorMemory(kSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kSize / detail::kBlockSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * detail::kBlockSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->blocks = 1;

        blocks.push_back(block);
    }

    for (size_t i = 0; i < blocks.size() - 1; i++) {
        blocks[i]->next = blocks[i + 1];
        blocks[i + 1]->prev = blocks[i];
    }

    detail::SwapAdjacentBlocks(blocks[1], blocks[2]);

    ASSERT_EQ(blocks[2]->next, blocks[1]);
    ASSERT_EQ(blocks[1]->prev, blocks[2]);

    ASSERT_EQ(blocks[3]->prev, blocks[1]);
    ASSERT_EQ(blocks[1]->next, blocks[3]);

    ASSERT_EQ(blocks[0]->next, blocks[2]);
    ASSERT_EQ(blocks[2]->prev, blocks[0]);
}

TEST(TableAllocatorDetailTest, SwapAnyBlocks) {
    TestMemory memory = GetAllocatorMemory(kSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kSize / detail::kBlockSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * detail::kBlockSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->blocks = 1;

        blocks.push_back(block);
    }

    for (size_t i = 0; i < blocks.size() - 1; i++) {
        blocks[i]->next = blocks[i + 1];
        blocks[i + 1]->prev = blocks[i];
    }

    detail::SwapAnyBlocks(blocks[1], blocks[4]);

    ASSERT_EQ(blocks[0]->next, blocks[4]);
    ASSERT_EQ(blocks[4]->prev, blocks[0]);

    ASSERT_EQ(blocks[2]->prev, blocks[4]);
    ASSERT_EQ(blocks[4]->next, blocks[2]);

    ASSERT_EQ(blocks[3]->next, blocks[1]);
    ASSERT_EQ(blocks[1]->prev, blocks[3]);

    ASSERT_EQ(blocks[5]->prev, blocks[1]);
    ASSERT_EQ(blocks[1]->next, blocks[5]);
}

TEST(TableAllocatorDetailTest, SortBlocks) {
    TestMemory memory = GetAllocatorMemory(kSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kSize / detail::kBlockSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * detail::kBlockSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->blocks = 1;

        blocks.push_back(block);
    }

    std::mt19937 gen { 0x1234 };
    std::shuffle(blocks.begin(), blocks.end(), gen);

    for (size_t i = 0; i < blocks.size() - 1; i++) {
        blocks[i]->next = blocks[i + 1];
        blocks[i + 1]->prev = blocks[i];
    }

    detail::ControlBlock *head = blocks[0];
    detail::SortBlocks(head);
    std::sort(blocks.begin(), blocks.end());
    detail::ControlBlock *block = head->head();
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(block->prev, nullptr);
    ASSERT_EQ((void*)block, memory.get());

    // ensure its sorted
    size_t count = 0;
    detail::ControlBlock *next = block;
    while (next != nullptr) {
        detail::ControlBlock *tmp = next->next;
        count += 1;
        if (tmp == nullptr) break;

        ASSERT_GT(tmp, next);
        next = tmp;
    }

    ASSERT_EQ(count, kSize / detail::kBlockSize);
}

TEST(TableAllocatorDetailTest, MergeBlocks) {
    TestMemory memory = GetAllocatorMemory(kSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kSize / detail::kBlockSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * detail::kBlockSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->blocks = 1;

        blocks.push_back(block);
    }

    for (size_t i = 0; i < blocks.size() - 1; i++) {
        blocks[i]->next = blocks[i + 1];
        blocks[i + 1]->prev = blocks[i];
    }

    detail::ControlBlock *head = blocks[0];
    detail::MergeAdjacentBlocks(head);

    // ensure everything has been merged
    ASSERT_EQ(head->blocks, kSize / detail::kBlockSize);
    ASSERT_EQ(head->next, nullptr);
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

        KmDebugMessage("Allocated: ", ptr, "\n");

        ASSERT_FALSE(ptrs.contains(ptr)) << "Duplicate pointer: " << ptr;

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

TEST(TableAllocateTest, Defragment) {
    size_t count = 16;
    size_t size = x64::kPageSize * count;
    TestMemory memory = GetAllocatorMemory(size);
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };

    std::mt19937 gen { 0x1234 };
    std::uniform_int_distribution<size_t> dist { 1, 4 };

    std::set<void*> ranges;
    std::map<void*, size_t> ptrs;
    for (size_t i = 0; i < count; i++) {
        size_t alloc = dist(gen);
        void *ptr = allocator.allocate(alloc);
        KmDebugMessage("Allocated: ", ptr, " size: ", alloc, "\n");
        if (ptr == nullptr) continue;

        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));
        ASSERT_FALSE(ranges.contains(ptr));

        ptrs[ptr] = alloc;

        for (size_t j = 0; j < alloc; j++) {
            ranges.insert((void*)((uintptr_t)ptr + (j * x64::kPageSize)));
        }
    }

    for (auto& [ptr, size] : ptrs) {
        KmDebugMessage("Deallocating: ", ptr, " size: ", size, "\n");
        allocator.deallocate(ptr, size);
    }

    // memory should be fragmented, so this will fail.
    // this detail function does not do defragmentation as part of its allocation
    void *ptr = detail::AllocateBlock(allocator, count);
    ASSERT_EQ(ptr, nullptr);

    // after defragmenting we should be able to allocate the whole block
    allocator.defragment();

    ptr = detail::AllocateBlock(allocator, count);
    IsValidPtr(ptr);
}
