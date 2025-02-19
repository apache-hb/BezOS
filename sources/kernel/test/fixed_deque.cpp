#include <gtest/gtest.h>
#include <random>

#include "std/fixed_deque.hpp"

template<typename T>
struct FixedDequeTestData {
    std::unique_ptr<T[]> data;
    stdx::FixedSizeDeque<T> ring;

    FixedDequeTestData(size_t size)
        : data(new T[size])
        , ring(data.get(), size)
    {
        std::fill_n(data.get(), size, 0xFAFAFAFA);
    }
};

TEST(FixedDequeTest, Construct) {
    FixedDequeTestData<int> test(16);

    ASSERT_EQ(test.ring.capacity(), 16);
    ASSERT_EQ(test.ring.count(), 0);
    ASSERT_TRUE(test.ring.isEmpty());
}

TEST(FixedDequeTest, AddBack) {
    FixedDequeTestData<int> test(16);

    // shift the head to the right
    test.ring.addFront(999);
    ASSERT_EQ(test.ring.front(), 999);
    ASSERT_EQ(test.ring.back(), 999);

    test.ring.addFront(999);
    ASSERT_EQ(test.ring.front(), 999);
    ASSERT_EQ(test.ring.back(), 999);

    test.ring.addFront(999);
    ASSERT_EQ(test.ring.front(), 999);
    ASSERT_EQ(test.ring.back(), 999);

    ASSERT_EQ(test.ring.pollBack(), 999);
    ASSERT_EQ(test.ring.pollBack(), 999);
    ASSERT_EQ(test.ring.pollBack(), 999);

    for (int i = 0; i < 16; i++) {
        ASSERT_TRUE(test.ring.addFront(i));
        ASSERT_EQ(test.ring.count(), i + 1) << "i = " << i;
    }

    ASSERT_TRUE(test.ring.isFull());

    ASSERT_FALSE(test.ring.addFront(16));
    ASSERT_EQ(test.ring.count(), 16);
}

TEST(FixedDequeTest, AddFront) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        ASSERT_TRUE(test.ring.addBack(i));
        ASSERT_EQ(test.ring.back(), i);
        ASSERT_EQ(test.ring.count(), i + 1);
    }

    ASSERT_FALSE(test.ring.addBack(16));
    ASSERT_EQ(test.ring.count(), 16);
}

TEST(FixedDequeTest, GetFront) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        test.ring.addFront(i);
    }

    for (int i = 0; i < 16; i++) {
        ASSERT_EQ(test.ring.pollBack(), i);
    }
}

TEST(FixedDequeTest, Circular) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 20; i++) {
        ASSERT_TRUE(test.ring.addFront(99 + i));
        ASSERT_EQ(test.ring.pollBack(), 99 + i) << "i = " << i;
    }
}

TEST(FixedDequeTest, GetBack) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        ASSERT_TRUE(test.ring.addFront(i));
        ASSERT_EQ(test.ring.front(), i);
    }

    for (int i = 15; i >= 0; i--) {
        ASSERT_EQ(test.ring.pollFront(), i);
    }
}

TEST(FixedDequeTest, AddRemove) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        test.ring.addFront(i);
    }

    for (int i = 0; i < 16; i++) {
        ASSERT_EQ(test.ring.pollBack(), i);
    }

    for (int i = 0; i < 16; i++) {
        test.ring.addFront(i);
    }

    for (int i = 0; i < 16; i++) {
        ASSERT_EQ(test.ring.pollBack(), i);
    }
}

TEST(FixedDequeTest, AddMixed) {
    // add and remove from both ends

    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 8; i++) {
        ASSERT_TRUE(test.ring.addFront(i));
    }

    for (int i = 0; i < 8; i++) {
        ASSERT_TRUE(test.ring.addBack(i + 8));
    }

    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(test.ring.pollFront(), 7 - i) << "i = " << i;
    }

    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(test.ring.pollBack(), 15 - i) << "i = " << i;
    }
}

TEST(FixedDequeTest, Iter) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        test.ring.addFront(i);
    }

    int i = 0;
    for (auto it : test.ring) {
        ASSERT_EQ(it, i) << "i = " << i;
        i++;
    }
}

TEST(FixedDequeTest, Erase) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        test.ring.addBack(i);
        ASSERT_EQ(test.ring.back(), i);
    }

    auto it = test.ring.begin();
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(*it, 15 - i) << "i = " << i;
        it++;
    }

    ASSERT_NE(test.ring.begin(), test.ring.end());

    it = test.ring.begin();
    int iter = 0;
    while (it != test.ring.end()) {
        if (iter % 2 == 0) {
            it = test.ring.erase(it);
            EXPECT_EQ(*it, 14 - iter) << "iter = " << iter;
        } else {
            it++;
        }

        iter++;
    }

    ASSERT_EQ(iter, 16);

    ASSERT_EQ(test.ring.count(), 8);

    it = test.ring.begin();
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(*it, 14 - (i * 2)) << "i = " << i;
        it++;
    }
}

TEST(FixedDequeTest, Interleaved) {
    std::unique_ptr<int[]> data(new int[16]);
    stdx::FixedSizeDeque<int> ring(data.get(), 16);

    for (int i = 0; i < 8; i++) {
        ring.addFront(i);
        ring.addBack(i + 8);
    }

    ASSERT_EQ(ring.front(), 7);

    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(ring.front(), 7 - i) << "i = " << i;
        ASSERT_TRUE(ring.popFront());

        EXPECT_EQ(ring.back(), 15 - i) << "i = " << i;
        ASSERT_TRUE(ring.popBack());
    }
}

TEST(FixedDequeTest, Sort) {
    FixedDequeTestData<int> test(256);

    std::mt19937 rng(0x1234);
    std::uniform_int_distribution<> dist(INT_MIN, INT_MAX);
    for (int i = 0; i < 256; i++) {
        test.ring.addFront(dist(rng));
    }

    std::sort(test.ring.begin(), test.ring.end());

    ASSERT_TRUE(std::is_sorted(test.ring.begin(), test.ring.end()));
}
