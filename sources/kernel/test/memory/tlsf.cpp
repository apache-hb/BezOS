#include <gtest/gtest.h>

#include "memory/detail/heap.hpp"
#include "test/new_shim.hpp"
#include "util/memory.hpp"

using km::TlsfHeap;
using km::TlsfAllocation;

class TlsfHeapDetailTest : public testing::Test {};

TEST_F(TlsfHeapDetailTest, BitScanLeading) {
    static_assert(km::detail::BitScanLeading(0u) == UINT8_MAX);
    static_assert(km::detail::BitScanLeading(0x1u) == 0);
    static_assert(km::detail::BitScanLeading(0x2u) == 1);

    ASSERT_EQ(km::detail::BitScanLeading(0u), UINT8_MAX);
    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(km::detail::BitScanLeading(1u << i), i);
    }
}

TEST_F(TlsfHeapDetailTest, BitScanTrailing) {
    static_assert(km::detail::BitScanTrailing(0u) == UINT8_MAX);
    static_assert(km::detail::BitScanTrailing(0x1u) == 0);
    static_assert(km::detail::BitScanTrailing(0x2u) == 1);

    ASSERT_EQ(km::detail::BitScanTrailing(0u), UINT8_MAX);
    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(km::detail::BitScanTrailing(1u << i), i);
    }
}

TEST_F(TlsfHeapDetailTest, GetFreeListSize) {
    ASSERT_EQ(km::detail::GetFreeListSize(0, 0), 0);
}

class TlsfHeapTest : public testing::Test {
public:
    void SetUp() override {
        auto *allocator = GetGlobalAllocator();
        allocator->reset();
    }

    void TearDown() override {
    }
};

TEST_F(TlsfHeapTest, Construct) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
}

TEST_F(TlsfHeapTest, ConstructOutOfMemory) {
    for (size_t i = 0; i < 5; i++) {
        GetGlobalAllocator()->mFailAfter = i;

        TlsfHeap heap;
        OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
        GetGlobalAllocator()->reset();

        EXPECT_TRUE(status == OsStatusOutOfMemory || status == OsStatusSuccess) << "Failed to allocate memory for heap";
        if (status == OsStatusSuccess) {
            heap.validate();
        }
    }
}

TEST_F(TlsfHeapTest, Alloc) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_TRUE(addr.isValid());
}

TEST_F(TlsfHeapTest, AllocMassive) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    TlsfAllocation addr = heap.aligned_alloc(0x10, sm::terabytes(2).bytes());
    EXPECT_FALSE(addr.isValid());
}

TEST_F(TlsfHeapTest, AllocMany) {
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create({0x1000, 0x2000}, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    for (size_t i = 0; i < (0x1000 / 0x100); i++) {
        TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }

    // memory should be exhausted
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_FALSE(addr.isValid());
}

TEST_F(TlsfHeapTest, AllocFree) {
    km::MemoryRange range{0x1000, 0x2000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
        pointers.insert(addr);
    }

    // memory should be exhausted
    TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
    EXPECT_FALSE(addr.isValid());

    // GetGlobalAllocator()->mNoAlloc = true;
    for (auto addr : pointers) {
        heap.free(addr);
    }
    // GetGlobalAllocator()->mNoAlloc = false;
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
        pointers.insert(addr);
    }
}

// free random elements
TEST_F(TlsfHeapTest, AllocFreeSlots) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);

    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t align = 1 << alignDistribution(random);
        size_t size = sizeDistribution(random) * align;
        TlsfAllocation addr = heap.aligned_alloc(align, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
        pointers.insert(addr);
    }

    TlsfAllocation alloc;
    for (size_t i = 0; i < 32; i++) {
        // select random element
        size_t index = random() % pointers.size();
        auto it = pointers.begin();
        std::advance(it, index);
        alloc = *it;
        EXPECT_TRUE(alloc.isValid());
        EXPECT_TRUE(pointers.contains(alloc)) << "Pointer not found: " << (void*)heap.addressOf(alloc).address;

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(alloc);
        GetGlobalAllocator()->mNoAlloc = false;
        pointers.erase(it);
    }

    GetGlobalAllocator()->mNoAlloc = true;
    for (auto addr : pointers) {
        heap.free(addr);
    }
    GetGlobalAllocator()->mNoAlloc = false;
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());

        ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
        pointers.insert(addr);
    }
}

TEST_F(TlsfHeapTest, AllocReverse) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t align = 1 << alignDistribution(random);
        size_t size = sizeDistribution(random) * align;
        TlsfAllocation addr = heap.aligned_alloc(align, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        pointers.push_back(addr);
    }

    for (auto it = pointers.rbegin(); it != pointers.rend(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }
}

TEST_F(TlsfHeapTest, AllocForward) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t align = 1 << alignDistribution(random);
        size_t size = sizeDistribution(random) * align;
        TlsfAllocation addr = heap.aligned_alloc(align, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        pointers.push_back(addr);
    }

    for (auto it = pointers.begin(); it != pointers.end(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }
}

TEST_F(TlsfHeapTest, AllocSizeVarying) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> sizeDistribution(2, 16);

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        size_t size = sizeDistribution(random) * 16 * 0x10;
        TlsfAllocation addr = heap.aligned_alloc(0x10, size);
        if (addr.isNull()) {
            continue;
        }

        EXPECT_TRUE(addr.isValid());

        pointers.push_back(addr);
    }

    for (auto it = pointers.begin(); it != pointers.end(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();

    // should be able to allocate again
    for (size_t i = 0; i < (range.size() / 0x100); i++) {
        auto addr = heap.aligned_alloc(0x10, 0x100);
        EXPECT_TRUE(addr.isValid());
    }
}

TEST_F(TlsfHeapTest, AllocLarge) {
    km::MemoryRange range{0x1000, 0x40000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto a1 = heap.aligned_alloc(0x1000, 0x8000);
    ASSERT_TRUE(a1.isValid());

    heap.free(a1);
}

TEST_F(TlsfHeapTest, AllocSeq) {
    km::MemoryRange range{0x1000, 0x40000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto a1 = heap.aligned_alloc(8, 16);
    ASSERT_TRUE(a1.isValid());

    auto a2 = heap.aligned_alloc(4, 12);
    ASSERT_TRUE(a2.isValid());

    auto a3 = heap.aligned_alloc(8, 24);
    ASSERT_TRUE(a3.isValid());

    heap.free(a2);
    heap.free(a3);
    heap.free(a1);

    auto a4 = heap.aligned_alloc(256, 2048 * 4);
    ASSERT_TRUE(a4.isValid());

    heap.free(a4);

    auto a5 = heap.aligned_alloc(8, 2048 * 4);
    ASSERT_TRUE(a5.isValid());
}

TEST_F(TlsfHeapTest, AllocPrevFree) {
    km::MemoryRange range{0x1000, 0x20000};
    TlsfHeap heap;
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);

    auto a1 = heap.aligned_alloc(0x10, 0x100);
    ASSERT_TRUE(a1.isValid());

    std::vector<TlsfAllocation> pointers;
    for (size_t i = 1; i < 6; i++) {
        auto addr = heap.aligned_alloc((1 << i), i * 0x100);
        ASSERT_TRUE(addr.isValid());
        pointers.push_back(addr);
    }

    for (auto it = pointers.rbegin(); it != pointers.rend(); ++it) {
        EXPECT_TRUE(it->isValid());

        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(*it);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
    heap.free(a1);

    auto a3 = heap.aligned_alloc((1 << 8), 0x800);
    ASSERT_TRUE(a3.isValid());

    heap.free(a3);
}

// random access allocations
TEST_F(TlsfHeapTest, AllocFreeRandom) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 100);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < 200; i++) {
        if (pointers.size() > 0 && distribution(random) < 50) {
            auto it = pointers.begin();
            std::advance(it, distribution(random) % pointers.size());

            GetGlobalAllocator()->mNoAlloc = true;
            heap.free(*it);
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(it);
        } else {
            TlsfAllocation addr = heap.aligned_alloc(0x10, 0x100);
            if (addr.isNull()) {
                continue;
            }

            EXPECT_TRUE(addr.isValid());

            ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
            pointers.insert(addr);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// random access allocations
TEST_F(TlsfHeapTest, AllocFreeRandomAlign) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 100);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::set<TlsfAllocation> pointers;
    for (size_t i = 0; i < 200; i++) {
        if (pointers.size() > 0 && distribution(random) < 50) {
            auto it = pointers.begin();
            std::advance(it, distribution(random) % pointers.size());

            GetGlobalAllocator()->mNoAlloc = true;
            heap.free(*it);
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(it);
        } else {
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            TlsfAllocation addr = heap.aligned_alloc(align, size);
            if (addr.isNull()) {
                continue;
            }

            EXPECT_TRUE(addr.isValid());

            ASSERT_FALSE(pointers.contains(addr)) << "Duplicate pointer: " << (void*)heap.addressOf(addr).address;
            pointers.insert(addr);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, AllocChunk) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            auto ptr = heap.aligned_alloc(0x10, 0x100);
            if (!ptr.isValid()) {
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)heap.addressOf(ptr).address;
            pointers.push_back(ptr);
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, AllocChunkRandomSize) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 1000; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            if (!ptr.isValid()) {
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)heap.addressOf(ptr).address;
            pointers.push_back(ptr);

            EXPECT_TRUE(heap.addressOf(ptr).address % align == 0) << "Pointer not aligned: " << align << " ptr " << (void*)heap.addressOf(ptr).address;
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, AllocChunkRandomSizeOom) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            GetGlobalAllocator()->mFailPercent = 0.5f;
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            GetGlobalAllocator()->mFailPercent = 0.f;

            if (!ptr.isValid()) {
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)heap.addressOf(ptr).address;
            pointers.push_back(ptr);

            EXPECT_TRUE(heap.addressOf(ptr).address % align == 0) << "Pointer not aligned: " << align << " ptr " << (void*)heap.addressOf(ptr).address;
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}

// test many consecutive allocations then consecutive frees
TEST_F(TlsfHeapTest, OomDoesntLeakBlocks) {
    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 128);
    std::uniform_int_distribution<size_t> alignDistribution(1, 8);
    std::uniform_int_distribution<size_t> sizeDistribution(2, 64);
    TlsfHeap heap;
    km::MemoryRange range{0x1000, 0x10000};
    OsStatus status = TlsfHeap::create(range, &heap);
    EXPECT_EQ(status, OsStatusSuccess);
    std::vector<TlsfAllocation> pointers;
    for (size_t i = 0; i < 500; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            km::TlsfHeapStats before = heap.stats();

            GetGlobalAllocator()->mFailPercent = 0.5f;
            size_t align = 1 << alignDistribution(random);
            size_t size = sizeDistribution(random) * align;
            auto ptr = heap.aligned_alloc(align, size);
            GetGlobalAllocator()->mFailPercent = 0.f;

            if (!ptr.isValid()) {
                km::TlsfHeapStats after = heap.stats();
                EXPECT_EQ(before.pool.freeSlots, after.pool.freeSlots) << "Free blocks should not change on OOM";
                ASSERT_NE(after.controlMemory, 0) << "Used memory should not be zero";
                break;
            }

            EXPECT_TRUE(ptr.isValid());
            ASSERT_TRUE(std::ranges::find(pointers, ptr) == pointers.end()) << "Duplicate pointer: " << (void*)heap.addressOf(ptr).address;
            pointers.push_back(ptr);
        }

        if (distribution(random) < 24) {
            GetGlobalAllocator()->mNoAlloc = true;
            size_t front = distribution(random) % pointers.size();
            size_t back = distribution(random) % pointers.size();
            if (front > back) {
                std::swap(front, back);
            }
            for (size_t j = front; j < back; j++) {
                heap.free(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }

    for (auto addr : pointers) {
        GetGlobalAllocator()->mNoAlloc = true;
        heap.free(addr);
        GetGlobalAllocator()->mNoAlloc = false;
    }
    pointers.clear();
}
