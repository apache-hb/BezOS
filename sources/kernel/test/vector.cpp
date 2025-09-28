#include <gtest/gtest.h>

#include "std/vector.hpp"

TEST(VectorTest, DefaultConstructor) {
    stdx::Vector2<int> vec;
    ASSERT_TRUE(vec.isEmpty());
    ASSERT_EQ(vec.count(), 0);
}

TEST(VectorTest, ConstructorWithCapacity) {
    stdx::Vector2<int> vec(10);
    ASSERT_TRUE(vec.isEmpty());
    ASSERT_EQ(vec.count(), 0);
    ASSERT_EQ(vec.capacity(), 10);
}

TEST(VectorTest, CopyConstructor) {
    stdx::Vector2<int> vec1{5};
    vec1.add(1);
    vec1.add(2);
    vec1.add(3);
    stdx::Vector2<int> vec2{vec1};
    ASSERT_EQ(vec2.count(), 3);
    ASSERT_GE(vec2.capacity(), 3);
    for (size_t i = 0; i < 3; ++i) {
        ASSERT_EQ(vec2[i], vec1[i]);
    }
}

TEST(VectorTest, MoveConstructor) {
    stdx::Vector2<int> vec1{5};
    vec1.add(1);
    vec1.add(2);
    vec1.add(3);
    stdx::Vector2<int> vec2{std::move(vec1)};
    ASSERT_EQ(vec2.count(), 3);
    ASSERT_EQ(vec2.capacity(), 5);
    ASSERT_TRUE(vec1.isEmpty());
}

TEST(VectorTest, Add) {
    stdx::Vector2<int> vec{2};
    vec.add(1);
    vec.add(2);
    vec.add(3);
    ASSERT_EQ(vec.count(), 3);
    ASSERT_GE(vec.capacity(), 3);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
}

TEST(VectorTest, AddRange) {
    stdx::Vector2<int> vec{2};
    int data[] = {1, 2, 3, 4, 5};
    std::span<const int> span(data, 5);
    vec.addRange(span);
    ASSERT_EQ(vec.count(), 5);
    ASSERT_GE(vec.capacity(), 5);
    for (size_t i = 0; i < 5; ++i) {
        ASSERT_EQ(vec[i], data[i]);
    }
}
