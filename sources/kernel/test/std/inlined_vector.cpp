#include <gtest/gtest.h>

#include "std/inlined_vector.hpp"
#include "new_shim.hpp"

class InlinedVectorTest : public testing::Test {
public:
    void SetUp() override {
        GetGlobalAllocator()->reset();
    }
};

TEST_F(InlinedVectorTest, Construct) {
    sm::InlinedVector<int, 4> vec;
    EXPECT_EQ(vec.begin(), vec.end());
    EXPECT_TRUE(vec.isEmpty());
    EXPECT_EQ(vec.count(), 0);
    EXPECT_EQ(vec.capacity(), 4);
}

TEST_F(InlinedVectorTest, Add) {
    sm::InlinedVector<int, 4> vec;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(vec.add(i), OsStatusSuccess);
    }
    EXPECT_EQ(vec.count(), 4);
    EXPECT_GE(vec.capacity(), 4);

    ASSERT_EQ(vec.add(4), OsStatusSuccess);
    EXPECT_EQ(vec.count(), 5);
    EXPECT_GT(vec.capacity(), 5);
}

TEST_F(InlinedVectorTest, AddOom) {
    sm::InlinedVector<int, 4> vec;
    for (int i = 0; i < 3; ++i) {
        GetGlobalAllocator()->mNoAlloc = true;
        OsStatus status = vec.add(i);
        GetGlobalAllocator()->mNoAlloc = false;

        ASSERT_EQ(status, OsStatusSuccess);
    }

    EXPECT_FALSE(vec.isHeapAllocated());

    GetGlobalAllocator()->mFailAfter = 0;
    GetGlobalAllocator()->mAllocCount = 1;
    OsStatus status = vec.add(4);
    GetGlobalAllocator()->mFailAfter = SIZE_MAX;

    EXPECT_EQ(status, OsStatusOutOfMemory);
    EXPECT_FALSE(vec.isHeapAllocated());

    EXPECT_EQ(vec.count(), 3);
    EXPECT_EQ(vec.capacity(), 4);

    ASSERT_EQ(vec.add(4), OsStatusSuccess);
    EXPECT_TRUE(vec.isHeapAllocated());
    EXPECT_EQ(vec.count(), 4);
    EXPECT_GT(vec.capacity(), 4);
}

TEST_F(InlinedVectorTest, Reserve) {
    sm::InlinedVector<int, 4> vec;
    ASSERT_EQ(vec.reserve(8), OsStatusSuccess);
    EXPECT_EQ(vec.count(), 0);
    EXPECT_EQ(vec.capacity(), 8);

    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(vec.add(i), OsStatusSuccess);
    }
    EXPECT_EQ(vec.count(), 8);
}

TEST_F(InlinedVectorTest, ReserveOom) {
    sm::InlinedVector<int, 4> vec;
    ASSERT_EQ(vec.reserve(8), OsStatusSuccess);
    EXPECT_EQ(vec.count(), 0);
    EXPECT_EQ(vec.capacity(), 8);

    GetGlobalAllocator()->mFailAfter = 0;
    OsStatus status = vec.reserve(16);
    GetGlobalAllocator()->mFailAfter = SIZE_MAX;

    EXPECT_EQ(status, OsStatusOutOfMemory);
    EXPECT_EQ(vec.count(), 0);
    EXPECT_EQ(vec.capacity(), 8);

    ASSERT_EQ(vec.reserve(16), OsStatusSuccess);
    EXPECT_EQ(vec.count(), 0);
    EXPECT_EQ(vec.capacity(), 16);
}

TEST_F(InlinedVectorTest, Resize) {
    sm::InlinedVector<int, 4> vec;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(vec.add(i), OsStatusSuccess);
    }

    EXPECT_EQ(vec.count(), 4);

    ASSERT_EQ(vec.resize(8), OsStatusSuccess);
    EXPECT_EQ(vec.count(), 8);

    ASSERT_EQ(vec.resize(2), OsStatusSuccess);
    EXPECT_EQ(vec.count(), 2);
}

TEST_F(InlinedVectorTest, ResizeOom) {
    sm::InlinedVector<int, 4> vec;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(vec.add(i), OsStatusSuccess);
    }

    EXPECT_EQ(vec.count(), 4);

    GetGlobalAllocator()->mFailAfter = 0;
    OsStatus status = vec.resize(8);
    GetGlobalAllocator()->mFailAfter = SIZE_MAX;

    ASSERT_EQ(status, OsStatusOutOfMemory);
    EXPECT_EQ(vec.count(), 4);

    ASSERT_EQ(vec.resize(2), OsStatusSuccess);
    EXPECT_EQ(vec.count(), 2);
}

TEST_F(InlinedVectorTest, Remove) {
    sm::InlinedVector<int, 4> vec;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(vec.add(i), OsStatusSuccess);
    }

    EXPECT_EQ(vec.count(), 4);

    vec.remove(2);
    EXPECT_EQ(vec.count(), 3);
    EXPECT_EQ(vec[0], 0);
    EXPECT_EQ(vec[1], 1);
    EXPECT_EQ(vec[2], 3);
}

TEST_F(InlinedVectorTest, RemoveFirst) {
    sm::InlinedVector<int, 4> vec;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(vec.add(i), OsStatusSuccess);
    }

    EXPECT_EQ(vec.count(), 4);

    vec.remove(0);
    EXPECT_EQ(vec.count(), 3);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
}

TEST_F(InlinedVectorTest, RemoveLast) {
    sm::InlinedVector<int, 4> vec;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(vec.add(i), OsStatusSuccess);
    }

    EXPECT_EQ(vec.count(), 4);

    vec.remove(3);
    EXPECT_EQ(vec.count(), 3);
    EXPECT_EQ(vec[0], 0);
    EXPECT_EQ(vec[1], 1);
    EXPECT_EQ(vec[2], 2);
}

TEST_F(InlinedVectorTest, ReserveMany) {
    sm::InlinedVector<int, 4> vec;
    ASSERT_EQ(vec.reserveExact(8), OsStatusSuccess);
    ASSERT_EQ(vec.reserveExact(16), OsStatusSuccess);
}
