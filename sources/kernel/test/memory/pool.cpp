#include <gtest/gtest.h>

#include "memory/detail/pool.hpp"
#include "new_shim.hpp"

template<typename T>
using Pool = km::PoolAllocator<T>;

class PoolTest : public testing::Test {
public:
    void SetUp() override {
        auto *allocator = GetGlobalAllocator();
        allocator->reset();
    }

    void TearDown() override {
    }
};

TEST_F(PoolTest, Construct) {
    // Constructor must never allocate
    GetGlobalAllocator()->mNoAlloc = true;
    Pool<int> pool;
    GetGlobalAllocator()->mNoAlloc = false;
}

TEST_F(PoolTest, Alloc) {
    Pool<int> pool;
    int *ptr = pool.construct(5);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 5);
    pool.release(ptr);
}

TEST_F(PoolTest, AllocMany) {
    Pool<int> pool;
    std::set<int*> pointers;

    for (size_t i = 0; i < 100; i++) {
        int *ptr = pool.construct(i);
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(*ptr, i);

        ASSERT_FALSE(pointers.contains(ptr)) << "Duplicate pointer: " << ptr;

        pointers.insert(ptr);
    }
}

TEST_F(PoolTest, AllocFree) {
    Pool<int> pool;
    std::set<int*> pointers;

    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 100);

    for (size_t i = 0; i < 100; i++) {
        if (pointers.size() > 0 && distribution(random) < 50) {
            auto it = pointers.begin();
            std::advance(it, distribution(random) % pointers.size());
            pool.destroy(*it);
            pointers.erase(it);
        } else {
            int *ptr = pool.construct(i);
            EXPECT_NE(ptr, nullptr);
            EXPECT_EQ(*ptr, i);

            ASSERT_FALSE(pointers.contains(ptr)) << "Duplicate pointer: " << ptr;

            pointers.insert(ptr);
        }
    }
}

TEST_F(PoolTest, AllocChunk) {
    Pool<int> pool;
    std::vector<int*> pointers;

    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 32);

    for (size_t i = 0; i < 100; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            int *ptr = pool.construct(i);
            EXPECT_NE(ptr, nullptr);
            EXPECT_EQ(*ptr, i);

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
                pool.destroy(pointers[j]);
            }
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }
}

TEST_F(PoolTest, CompactChunk) {
    Pool<int> pool;
    std::vector<int*> pointers;

    std::mt19937 random{0x1234};
    std::uniform_int_distribution<size_t> distribution(0, 32);

    for (size_t i = 0; i < 100; i++) {
        for (size_t j = 0; j < distribution(random); j++) {
            int *ptr = pool.construct(i);
            EXPECT_NE(ptr, nullptr);
            EXPECT_EQ(*ptr, i);

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
                pool.destroy(pointers[j]);
            }
            pool.compact();
            GetGlobalAllocator()->mNoAlloc = false;

            pointers.erase(pointers.begin() + front, pointers.begin() + back);
        }
    }
}

TEST_F(PoolTest, Stats) {
    Pool<int> pool;
    auto s0 = pool.stats();
    std::set<int*> pointers;
    for (size_t i = 0; i < 100; i++) {
        pointers.insert(pool.construct(i));
    }
    auto s1 = pool.stats();

    ASSERT_NE(s1.magazines, 0) << "Magazines should be allocated";
    ASSERT_NE(s1.totalSlots, 0) << "Free slots should be allocated";

    ASSERT_GT(s1.magazines, s0.magazines) << "Magazines should be allocated as needed";
    ASSERT_GT(s1.totalSlots, s0.totalSlots) << "Total slots increase with allocations";

    for (int *ptr : pointers) {
        pool.destroy(ptr);
    }
    pointers.clear();
    auto s2 = pool.stats();

    ASSERT_EQ(s2.magazines, s1.magazines) << "Magazines should not be released until compaction";
    ASSERT_EQ(s2.totalSlots, s1.totalSlots) << "Total slots should not change after deallocation";
    ASSERT_GT(s2.freeSlots, s1.freeSlots) << "Free slots should increase after deallocation";
}

TEST_F(PoolTest, Compact) {
    Pool<int> pool;
    std::set<int*> pointers;

    for (size_t i = 0; i < 100; i++) {
        int *ptr = pool.construct(i);
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(*ptr, i);

        ASSERT_FALSE(pointers.contains(ptr)) << "Duplicate pointer: " << ptr;

        pointers.insert(ptr);
    }

    for (int *ptr : pointers) {
        pool.destroy(ptr);
    }

    auto s1 = pool.stats();
    pool.compact();
    auto s2 = pool.stats();

    ASSERT_LT(s2.magazines, s1.magazines) << "Magazines should be released after compaction";
    ASSERT_LT(s2.totalSlots, s1.totalSlots) << "Total slots should be released after compaction";
}

TEST_F(PoolTest, OutOfMemory) {
    std::set<int*> pointers;
    Pool<int> pool;

    for (size_t i = 0; i < 100; i++) {

        GetGlobalAllocator()->mFailPercent = 0.25f;
        int *ptr = pool.construct(i);
        GetGlobalAllocator()->mFailPercent = 0.0f;

        if (ptr == nullptr) {
            continue;
        }

        EXPECT_EQ(*ptr, i);

        ASSERT_FALSE(pointers.contains(ptr)) << "Duplicate pointer: " << ptr;

        pointers.insert(ptr);
    }

    GetGlobalAllocator()->mFailPercent = 0.25f;
    for (int *ptr : pointers) {
        pool.destroy(ptr);
    }

    auto s1 = pool.stats();
    pool.compact();
    auto s2 = pool.stats();
    GetGlobalAllocator()->mFailPercent = 0.0f;

    ASSERT_LT(s2.magazines, s1.magazines) << "Magazines should be released after compaction";
    ASSERT_LT(s2.totalSlots, s1.totalSlots) << "Total slots should be released after compaction";
}
