#include <gtest/gtest.h>

#include "std/fixed_deque.hpp"

template<typename T>
struct FixedDequeTestData {
    std::unique_ptr<T[]> data;
    stdx::FixedDeque<T> ring;

    FixedDequeTestData(size_t size)
        : data(new T[size])
        , ring(data.get(), data.get() + size)
    {
        memset(data.get(), 0xFA, size);
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
    test.ring.addFront(999);
    test.ring.addFront(999);
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
        test.ring.addFront(i);
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

TEST(FixedDequeTest, Interleaved) {
    // add and remove from both ends

    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 8; i++) {
        test.ring.addFront(i);
        test.ring.addBack(i + 8);
    }

    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(test.ring.pollBack(), 15 - i) << "i = " << i;
        EXPECT_EQ(test.ring.pollFront(), 7 - i) << "i = " << i;
    }
}

TEST(FixedDequeTest, Iter) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        test.ring.addFront(i);
    }

    int i = 0;
    for (auto it : test.ring) {
        ASSERT_EQ(it, 15 - i) << "i = " << i;
        i++;
    }
}

TEST(FixedDequeTest, Erase) {
    FixedDequeTestData<int> test(16);

    for (int i = 0; i < 16; i++) {
        test.ring.addFront(i);
        ASSERT_EQ(test.ring.front(), i);
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
            ASSERT_EQ(*it, 15 - iter) << "iter = " << iter;
        } else {
            it++;
        }

        iter++;
    }

    ASSERT_EQ(iter, 16);

    ASSERT_EQ(test.ring.count(), 8);

    it = test.ring.begin();
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(*it, 15 - (i * 2)) << "i = " << i;
        it++;
    }
}
