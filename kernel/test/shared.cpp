#include <gtest/gtest.h>

#include "std/shared.hpp"

TEST(SharedPtrTest, ConstructNull) {
    sm::SharedPtr<int> ptr;
}

TEST(SharedPtrTest, Construct) {
    sm::SharedPtr<int> ptr(new int(5));
    ASSERT_EQ(*ptr, 5);
}

TEST(SharedPtrTest, Copy) {
    sm::SharedPtr<int> ptr(new int(5));
    sm::SharedPtr<int> ptr2(ptr);
    ASSERT_EQ(*ptr2, 5);
    ASSERT_EQ(ptr, ptr2);
}

TEST(SharedPtrTest, Move) {
    sm::SharedPtr<int> ptr(new int(5));
    sm::SharedPtr<int> ptr2(std::move(ptr));
    ASSERT_EQ(*ptr2, 5);
    ASSERT_EQ(ptr, nullptr);
}

TEST(SharedPtrTest, Assign) {
    sm::SharedPtr<int> ptr(new int(5));
    sm::SharedPtr<int> ptr2;
    ptr2 = ptr;
    ASSERT_EQ(*ptr2, 5);
    ASSERT_EQ(ptr, ptr2);
}

TEST(SharedPtrTest, AssignMove) {
    sm::SharedPtr<int> ptr(new int(5));
    sm::SharedPtr<int> ptr2;
    ptr2 = std::move(ptr);
    ASSERT_EQ(*ptr2, 5);
    ASSERT_EQ(ptr, nullptr);
}
