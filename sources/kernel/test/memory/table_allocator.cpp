#include <gtest/gtest.h>

#include <random>

#include "memory/table_allocator.hpp"

using namespace km;

static constexpr size_t kCount = 16;
static constexpr size_t kSize = kCount * x64::kPageSize;

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

static void IsValidPtr(void *ptr) {
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % x64::kPageSize, 0);
    memset(ptr, 0xFA, x64::kPageSize); // touch the pages to ensure they are mapped
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
    detail::sortBlocks(head);
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
    detail::sortBlocks(head);
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
    detail::sortBlocks(head);
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
        *block = {
            .next = nullptr,
            .prev = nullptr,
            .size = x64::kPageSize,
        };

        blocks.push_back(block);
    }

    for (size_t i = 0; i < blocks.size() - 1; i++) {
        blocks[i]->next = blocks[i + 1];
        blocks[i + 1]->prev = blocks[i];
    }

    detail::ControlBlock *head = blocks[0];
    detail::mergeAdjacentBlocks(head);

    // ensure everything has been merged
    ASSERT_EQ(head->size, kSize);
    ASSERT_EQ(head->next, nullptr);
}

TEST(TableAllocatorConstructTest, Construct) {
    TestMemory memory = GetAllocatorMemory();
    PageTableAllocator allocator;
    OsStatus status = PageTableAllocator::create(VirtualRangeEx::of(memory.get(), kSize), x64::kPageSize, &allocator);
    ASSERT_EQ(status, OsStatusSuccess);
}

class TableAllocatorTest : public testing::Test {
public:
    void SetUp() override {
        memory = GetAllocatorMemory();
        OsStatus status = PageTableAllocator::create(VirtualRangeEx::of(memory.get(), kSize), x64::kPageSize, &allocator);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    TestMemory memory;
    PageTableAllocator allocator;
};

TEST_F(TableAllocatorTest, Allocate) {
    void *ptr = allocator.allocate(1).getVirtual();
    IsValidPtr(ptr);

    allocator.deallocate(ptr, 1, 0);
}

TEST_F(TableAllocatorTest, AllocateMany) {
    size_t count = kCount;

    std::set<void*> ptrs;
    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1).getVirtual();
        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr)) << "Duplicate pointer: " << ptr;

        ptrs.insert(ptr);
    }

    for (void *ptr : ptrs) {
        allocator.deallocate(ptr, 1);
    }

    ptrs.clear();

    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1).getVirtual();
        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));

        ptrs.insert(ptr);
    }

    for (void *ptr : ptrs) {
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AllocateOom) {
    size_t count = kCount;
    std::set<void*> ptrs;
    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1).getVirtual();
        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));

        ptrs.insert(ptr);
    }

    for (size_t i = 0; i < 10; i++) {
        void *ptr = allocator.allocate(1).getVirtual();
        ASSERT_EQ(ptr, nullptr);
    }

    for (void *ptr : ptrs) {
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AllocateSized) {
    size_t count = kCount;
    std::mt19937 gen { 0x1234 };
    std::uniform_int_distribution<size_t> dist { 1, 4 };

    std::map<void*, size_t> ptrs;
    for (size_t i = 0; i < count; i++) {
        size_t alloc = dist(gen);
        void *ptr = allocator.allocate(alloc).getVirtual();
        if (ptr == nullptr) continue;

        IsValidPtr(ptr);

        ASSERT_FALSE(ptrs.contains(ptr));

        ptrs[ptr] = alloc;
    }

    for (auto& [ptr, size] : ptrs) {
        allocator.deallocate(ptr, size);
    }
}

TEST_F(TableAllocatorTest, PartialDeallocate) {
    size_t count = kCount;
    void *ptr = allocator.allocate(4).getVirtual();
    IsValidPtr(ptr);

    void *middle = (void*)((uintptr_t)ptr + x64::kPageSize);
    allocator.deallocate(middle, 1);

    void *next = (void*)((uintptr_t)middle + x64::kPageSize);
    allocator.deallocate(next, 2);

    allocator.deallocate(ptr, 1);

    for (size_t i = 0; i < count; i++) {
        void *ptr = allocator.allocate(1).getVirtual();
        IsValidPtr(ptr);

        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, Defragment) {
    size_t count = kCount;

    std::mt19937 gen { 0x1234 };
    std::uniform_int_distribution<size_t> dist { 1, 4 };

    std::set<void*> ranges;
    std::map<void*, size_t> ptrs;
    for (size_t i = 0; i < count; i++) {
        size_t alloc = dist(gen);
        void *ptr = allocator.allocate(alloc).getVirtual();
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

TEST_F(TableAllocatorTest, AllocateList) {
    detail::PageTableList list;
    size_t count = 8;
    ASSERT_TRUE(allocator.allocateList(count, &list));

    for (size_t i = 0; i < count; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AllocateListFull) {
    constexpr size_t kBlockCount = kCount;
    detail::PageTableList list;
    size_t count = kBlockCount;
    ASSERT_TRUE(allocator.allocateList(count, &list));

    for (size_t i = 0; i < count; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, CanAllocateBlocksFalse) {
    ASSERT_FALSE(detail::CanAllocateBlocks(allocator.TESTING_getHead(), 32 * x64::kPageSize));

    // failed allocation doesnt leak memory
    {
        detail::PageTableList list;
        size_t count = 32;
        ASSERT_FALSE(allocator.allocateList(count, &list));
    }

    detail::PageTableList list;
    size_t count = 16;
    ASSERT_TRUE(allocator.allocateList(count, &list));

    for (size_t i = 0; i < count; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AllocateListFails) {
    // failed allocation doesnt leak memory
    {
        detail::PageTableList list;
        size_t count = 32;
        ASSERT_FALSE(allocator.allocateList(count, &list));
    }

    detail::PageTableList list;
    size_t count = 16;
    ASSERT_TRUE(allocator.allocateList(count, &list));

    for (size_t i = 0; i < count; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AllocateListFragmented) {
    constexpr size_t kBlockCount = kCount;
    void *blocks[kBlockCount]{};
    for (size_t i = 0; i < kBlockCount; i++) {
        blocks[i] = allocator.allocate(1).getVirtual();
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
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AllocateListFragmented2) {
    constexpr size_t kBlockCount = kCount;
    void *blocks[kBlockCount]{};
    for (size_t i = 0; i < kBlockCount; i++) {
        blocks[i] = allocator.allocate(1).getVirtual();
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
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AllocateListFragmented3) {
    constexpr size_t kBlockCount = kCount;

    void *blocks[kBlockCount]{};
    for (size_t i = 0; i < kBlockCount; i++) {
        blocks[i] = allocator.allocate(1).getVirtual();
        IsValidPtr(blocks[i]);
    }

    allocator.deallocate(blocks[0], 1);

    allocator.deallocate(blocks[3], 1);
    allocator.deallocate(blocks[4], 1);
    allocator.deallocate(blocks[5], 1);
    allocator.deallocate(blocks[6], 1);
    allocator.deallocate(blocks[7], 1);
    allocator.deallocate(blocks[8], 1);
    allocator.deallocate(blocks[9], 1);
    allocator.deallocate(blocks[10], 1);
    allocator.deallocate(blocks[11], 1);

    allocator.defragment();

    detail::PageTableList list;
    ASSERT_TRUE(allocator.allocateList(3, &list));

    for (size_t i = 0; i < 3; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, AddMemory) {
    TestMemory extra = GetAllocatorMemory();
    allocator.addMemory(VirtualRangeEx::of(extra.get(), kSize));

    size_t totalBlocks = (kSize / x64::kPageSize) * 2;
    detail::PageTableList list;
    ASSERT_TRUE(allocator.allocateList(totalBlocks, &list));

    for (size_t i = 0; i < totalBlocks; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }
}

TEST_F(TableAllocatorTest, ReleaseMemory) {
    TestMemory extra = GetAllocatorMemory();
    allocator.addMemory(VirtualRangeEx::of(extra.get(), kSize));

    size_t totalBlocks = (kSize / x64::kPageSize) * 2;
    detail::PageTableList list;
    ASSERT_TRUE(allocator.allocateList(totalBlocks, &list));

    for (size_t i = 0; i < totalBlocks; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }

    allocator.releaseMemory(VirtualRangeEx::of(extra.get(), kSize));

    auto stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, 0);
}

TEST_F(TableAllocatorTest, AddMemoryMultiple) {
    std::vector<TestMemory> extras;
    for (size_t i = 0; i < 4; i++) {
        TestMemory extra = GetAllocatorMemory();
        allocator.addMemory(VirtualRangeEx::of(extra.get(), kSize));
        extras.push_back(std::move(extra));
    }

    size_t totalBlocks = (kSize / x64::kPageSize) * 5;
    detail::PageTableList list;
    ASSERT_TRUE(allocator.allocateList(totalBlocks, &list));

    for (size_t i = 0; i < totalBlocks; i++) {
        void *ptr = list.next().getVirtual();
        IsValidPtr(ptr);
        allocator.deallocate(ptr, 1);
    }

    auto stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 5);
}

TEST_F(TableAllocatorTest, ReleaseMemoryMultiple) {
    std::vector<TestMemory> extras;
    for (size_t i = 0; i < 5; i++) {
        TestMemory extra = GetAllocatorMemory();
        allocator.addMemory(VirtualRangeEx::of(extra.get(), kSize));
        extras.push_back(std::move(extra));
    }

    auto stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 6);

    allocator.dump();

    allocator.releaseMemory(VirtualRangeEx::of(extras[0].get(), kSize));

    allocator.dump();

    stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 5);

    allocator.releaseMemory(VirtualRangeEx::of(extras[1].get(), kSize / 2));

    stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 4 + (kCount / 2));

    allocator.releaseMemory(VirtualRangeEx::of(extras[3].get() + (kSize / 2), kSize / 2));

    stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 4);

    allocator.releaseMemory(VirtualRangeEx::of(extras[2].get() + (kSize / 4), (kSize / 2)));

    stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 3 + (kCount / 2));
}

TEST_F(TableAllocatorTest, ReleaseMemoryMultiple2) {
    std::vector<TestMemory> extras;
    for (size_t i = 0; i < 4; i++) {
        TestMemory extra = GetAllocatorMemory();
        allocator.addMemory(VirtualRangeEx::of(extra.get(), kSize));
        extras.push_back(std::move(extra));
    }

    auto stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 5);

    allocator.releaseMemory(VirtualRangeEx::of(extras[1].get() - (kSize / 2), kSize / 2));
    allocator.releaseMemory(VirtualRangeEx::of(extras[3].get() + (kSize / 2), kSize));
    allocator.releaseMemory(VirtualRangeEx::of(extras[0].get() + (kSize / 4), kSize));
    allocator.releaseMemory(VirtualRangeEx::of(extras[2].get() + (kSize / 4), (kSize / 4) * 3));
}

TEST_F(TableAllocatorTest, ReleaseMemoryThenAllocate) {
    std::vector<TestMemory> extras;
    for (size_t i = 0; i < 4; i++) {
        TestMemory extra = GetAllocatorMemory();
        printf("Adding memory %p - %p\n", extra.get(), (uint8_t*)extra.get() + kSize);
        allocator.addMemory(VirtualRangeEx::of(extra.get(), kSize));
        extras.push_back(std::move(extra));
    }

    auto stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 5);
    uint8_t *extra1 = extras[1].get();

    allocator.releaseMemory(VirtualRangeEx::of(extra1, kSize));

    stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 4);

    std::vector<void*> ptrs{};
    while (true) {
        PageTableAllocation alloc = allocator.allocate(1);
        if (!alloc.isPresent()) {
            printf("Allocation returned nullptr\n");
            break;
        }

        IsValidPtr(alloc.getVirtual());

        printf("Got ptr %p - %p\n", alloc.getVirtual(), (uint8_t*)alloc.getVirtual() + x64::kPageSize);

        ptrs.push_back(alloc.getVirtual());
    }

    stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, 0);

    ASSERT_EQ(ptrs.size(), kCount * 4);
    for (void *ptr : ptrs) {
        if (extra1 <= (uint8_t*)ptr && (uint8_t*)ptr < (extra1 + kSize)) {
            ASSERT_FALSE(true) << "Allocated from released memory: " << ptr;
        }

        allocator.deallocate(ptr, 1);
    }

    stats = allocator.stats();
    ASSERT_EQ(stats.freeBlocks, kCount * 4);
}
