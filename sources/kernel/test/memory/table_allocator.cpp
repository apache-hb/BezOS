#include <gtest/gtest.h>

#include <random>

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

TEST(TableAllocatorDetailTest, SingleBlock) {
    static constexpr size_t kListSize = x64::kPageSize;
    TestMemory memory = GetAllocatorMemory(kListSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kListSize / x64::kPageSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * x64::kPageSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->size = x64::kPageSize;

        blocks.push_back(block);
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

    ASSERT_EQ(count, kListSize / x64::kPageSize);
}

TEST(TableAllocatorDetailTest, TwoBlocks) {
    static constexpr size_t kListSize = 2 * x64::kPageSize;
    TestMemory memory = GetAllocatorMemory(kListSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kListSize / x64::kPageSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * x64::kPageSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->size = x64::kPageSize;

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

    ASSERT_EQ(count, kListSize / x64::kPageSize);
}

TEST(TableAllocatorDetailTest, SortBlocks) {
    TestMemory memory = GetAllocatorMemory(kSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kSize / x64::kPageSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * x64::kPageSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->size = x64::kPageSize;

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

    ASSERT_EQ(count, kSize / x64::kPageSize);
}

TEST(TableAllocatorDetailTest, MergeBlocks) {
    TestMemory memory = GetAllocatorMemory(kSize);

    std::vector<detail::ControlBlock*> blocks;

    for (size_t i = 0; i < (kSize / x64::kPageSize); i++) {
        detail::ControlBlock *block = (detail::ControlBlock*)(memory.get() + (i * x64::kPageSize));
        block->next = nullptr;
        block->prev = nullptr;
        block->size = x64::kPageSize;

        blocks.push_back(block);
    }

    for (size_t i = 0; i < blocks.size() - 1; i++) {
        blocks[i]->next = blocks[i + 1];
        blocks[i + 1]->prev = blocks[i];
    }

    detail::ControlBlock *head = blocks[0];
    detail::MergeAdjacentBlocks(head);

    // ensure everything has been merged
    ASSERT_EQ(head->size, kSize);
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
    void *ptr = detail::AllocateBlock(allocator, count * x64::kPageSize);
    ASSERT_EQ(ptr, nullptr);

    // after defragmenting we should be able to allocate the whole block
    allocator.defragment();

    ptr = detail::AllocateBlock(allocator, count * x64::kPageSize);
    IsValidPtr(ptr);
}

TEST(TableAllocatorTest, AllocateList) {
    TestMemory memory = GetAllocatorMemory();
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kSize) };

    detail::PageTableList list;
    size_t count = 8;
    ASSERT_TRUE(allocator.allocateList(count, &list));

    for (size_t i = 0; i < count; i++) {
        void *ptr = list.next();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST(TableAllocatorTest, AllocateListFragmented) {
    constexpr size_t kBlockCount = 16;
    constexpr size_t kMemorySize = kBlockCount * x64::kPageSize;
    TestMemory memory = GetAllocatorMemory(kMemorySize);
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kMemorySize) };

    void *blocks[kBlockCount]{};
    for (size_t i = 0; i < kBlockCount; i++) {
        blocks[i] = allocator.allocate(1);
        IsValidPtr(blocks[i]);
    }

    // free each alternate block
    for (size_t i = 0; i < kBlockCount; i += 2) {
        allocator.deallocate(blocks[i], 1);
    }

    auto stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kBlockCount / 2);

    // we should be able to allocate a list of 8 blocks
    detail::PageTableList list;
    ASSERT_TRUE(allocator.allocateList(8, &list));

    for (size_t i = 0; i < 8; i++) {
        void *ptr = list.next();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST(TableAllocatorTest, AllocateListFragmented2) {
    constexpr size_t kBlockCount = 16;
    constexpr size_t kMemorySize = kBlockCount * x64::kPageSize;
    TestMemory memory = GetAllocatorMemory(kMemorySize);
    PageTableAllocator allocator { VirtualRange::of(memory.get(), kMemorySize) };

    void *blocks[kBlockCount]{};
    for (size_t i = 0; i < kBlockCount; i++) {
        blocks[i] = allocator.allocate(1);
        IsValidPtr(blocks[i]);
    }

    allocator.deallocate(blocks[0], 1);
    allocator.deallocate(blocks[1], 1);

    allocator.deallocate(blocks[3], 1);
    allocator.deallocate(blocks[4], 1);

    allocator.deallocate(blocks[6], 1);
    allocator.deallocate(blocks[7], 1);

    allocator.deallocate(blocks[9], 1);
    allocator.deallocate(blocks[10], 1);
    allocator.deallocate(blocks[11], 1);

    allocator.defragment();

    auto stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, 9);

    // we should be able to allocate a list of 8 blocks
    detail::PageTableList list;
    ASSERT_TRUE(allocator.allocateList(9, &list));

    for (size_t i = 0; i < 9; i++) {
        void *ptr = list.next();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}
