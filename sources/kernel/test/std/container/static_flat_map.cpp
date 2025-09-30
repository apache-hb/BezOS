#include <gtest/gtest.h>

#include "std/container/static_flat_map.hpp"

using sm::StaticFlatMap;

using FlatMap = StaticFlatMap<int, int>;

TEST(StaticFlatMapConstructTest, Construct) {
    size_t required = FlatMap::computeRequiredStorage(10);
    std::unique_ptr<uint8_t[]> storage{new uint8_t[required]};
    FlatMap map{storage.get(), required};
    ASSERT_EQ(map.capacity(), 10);
    ASSERT_EQ(map.count(), 0);
    ASSERT_TRUE(map.isEmpty());
    ASSERT_FALSE(map.isFull());
}

class StaticFlatMapTest : public testing::Test {
public:
    static constexpr size_t kCapacity = 10;
    std::unique_ptr<uint8_t[]> storage;
    FlatMap map;

    void SetUp() override {
        size_t required = FlatMap::computeRequiredStorage(kCapacity);
        storage.reset(new uint8_t[required]);
        map = FlatMap{storage.get(), required};
        ASSERT_EQ(map.capacity(), kCapacity);
        ASSERT_EQ(map.count(), 0);
        ASSERT_TRUE(map.isEmpty());
        ASSERT_FALSE(map.isFull());
    }
};

TEST_F(StaticFlatMapTest, InsertAndFind) {
    OsStatus status = OsStatusSuccess;

    status = map.insert(1, 10);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(map.count(), 1);

    ASSERT_EQ(10, *map.find(1));
    ASSERT_EQ(nullptr, map.find(2));
}

TEST_F(StaticFlatMapTest, InsertUntilFull) {
    OsStatus status = OsStatusSuccess;

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(i, i * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(i + 1));
    }

    ASSERT_TRUE(map.isFull());
    ASSERT_FALSE(map.isEmpty());

    status = map.insert(100, 1000);
    ASSERT_EQ(status, OsStatusOutOfMemory);
    ASSERT_EQ(map.count(), kCapacity);
}

TEST_F(StaticFlatMapTest, InsertRandom) {
    OsStatus status = OsStatusSuccess;

    int keys[] = {5, 3, 8, 1, 4, 7, 2, 6, 0, 9};

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(keys[i], keys[i] * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(i + 1));
    }

    ASSERT_TRUE(map.isFull());
    ASSERT_FALSE(map.isEmpty());

    status = map.insert(100, 1000);
    ASSERT_EQ(status, OsStatusOutOfMemory);
    ASSERT_EQ(map.count(), kCapacity);
}

TEST_F(StaticFlatMapTest, InsertDuplicate) {
    OsStatus status = OsStatusSuccess;

    int keys[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(keys[i], keys[i] * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(1, map.count());
    }

    ASSERT_EQ(1, map.count());
}

TEST_F(StaticFlatMapTest, Remove) {
    OsStatus status = OsStatusSuccess;

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(i, i * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(i + 1));
    }

    ASSERT_TRUE(map.isFull());
    ASSERT_FALSE(map.isEmpty());

    status = map.insert(100, 1000);
    ASSERT_EQ(status, OsStatusOutOfMemory);
    ASSERT_EQ(map.count(), kCapacity);

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.remove(i);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(kCapacity - i - 1));
    }

    ASSERT_TRUE(map.isEmpty());
    ASSERT_FALSE(map.isFull());
    ASSERT_EQ(0, map.count());

    status = map.remove(100);
    ASSERT_EQ(status, OsStatusNotFound);
}

TEST_F(StaticFlatMapTest, Iterate) {
    OsStatus status = OsStatusSuccess;

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(i, i * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(i + 1));
    }

    size_t count = 0;
    for (FlatMap::Iterator it = map.begin(); it != map.end(); ++it) {
        int key = it.key();
        int value = it.value();
        ASSERT_EQ(value, key * 10);
        count++;
    }

    ASSERT_EQ(count, kCapacity);
}

TEST_F(StaticFlatMapTest, IterateCxx11) {
    OsStatus status = OsStatusSuccess;

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(i, i * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(i + 1));
    }

    size_t count = 0;
    for (const auto& [key, value] : map) {
        ASSERT_EQ(value, key * 10);
        count++;
    }

    ASSERT_EQ(count, kCapacity);
}

TEST_F(StaticFlatMapTest, IterateEmpty) {
    size_t count = 0;
    for (FlatMap::Iterator it = map.begin(); it != map.end(); ++it) {
        count++;
    }

    ASSERT_EQ(count, 0);
}

TEST_F(StaticFlatMapTest, Clear) {
    OsStatus status = OsStatusSuccess;

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(i, i * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(i + 1));
    }

    ASSERT_TRUE(map.isFull());
    ASSERT_FALSE(map.isEmpty());

    map.clear();
    ASSERT_TRUE(map.isEmpty());
    ASSERT_FALSE(map.isFull());
    ASSERT_EQ(0, map.count());

    for (size_t i = 0; i < kCapacity; i++) {
        status = map.insert(i, i * 10);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_EQ(map.count(), (size_t)(i + 1));
    }

    ASSERT_TRUE(map.isFull());
    ASSERT_FALSE(map.isEmpty());
}
