#include <gtest/gtest.h>

#include "std/static_vector.hpp"

TEST(StaticVectorTest, Construct) {
    stdx::StaticVector<int, 4> vec;

    vec.add(1);

    ASSERT_EQ(vec.count(), 1);
    ASSERT_EQ(vec[0], 1);
}

TEST(StaticVectorTest, AddRange) {
    stdx::StaticVector<int, 4> vec;

    int data[] = { 1, 2, 3, 4 };
    vec.addRange(data);

    ASSERT_EQ(vec.count(), 4);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
}

TEST(StaticVectorTest, AddOverflow) {
    stdx::StaticVector<int, 4> vec;

    int data[] = { 1, 2, 3, 4, 5, 6 };
    size_t actual = vec.addRange(data);

    ASSERT_EQ(vec.count(), 4);
    ASSERT_EQ(actual, 4);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
}

TEST(StaticVectorTest, AddVector) {
    stdx::StaticVector<int, 16> vec;
    stdx::StaticVector<int, 4> vec2;

    int data[] = { 1, 2, 3, 4 };
    vec2.addRange(data);

    vec.addRange(vec2);
    vec.addRange(vec2);

    ASSERT_EQ(vec.count(), 8);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
    ASSERT_EQ(vec[4], 1);
    ASSERT_EQ(vec[5], 2);
    ASSERT_EQ(vec[6], 3);
    ASSERT_EQ(vec[7], 4);
}
