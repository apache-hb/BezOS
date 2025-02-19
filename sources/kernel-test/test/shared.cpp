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

TEST(SharedPtrTest, WeakPtr) {
    sm::WeakPtr<int> weak;

    {
        sm::SharedPtr<int> ptr(new int(5));

        weak = ptr.weak();
    }

    ASSERT_EQ(weak.lock(), nullptr);
}

struct Data {
    std::vector<sm::SharedPtr<Data>> children;
};

TEST(SharedPtrTest, RecurisveData) {
    sm::SharedPtr<Data> root(new Data());
    sm::SharedPtr<Data> child0(new Data());
    sm::SharedPtr<Data> child1(new Data());

    root->children.push_back(child0);
    child0->children.push_back(child1);
    root->children.push_back(child1);
}

TEST(SharedPtrTest, WeakPtrCopy) {
    sm::SharedPtr<int> ptr(new int(5));
    sm::WeakPtr<int> weak(ptr);

    sm::WeakPtr<int> weak2(weak);
    ASSERT_EQ(*weak.lock(), 5);
    ASSERT_EQ(*weak2.lock(), 5);
}

struct Counted : public sm::IntrusiveCount<Counted> {
    std::vector<sm::SharedPtr<Counted>> children;
};

TEST(SharedPtrTest, IntrusiveCount) {
    sm::SharedPtr<Counted> root(new Counted());
    sm::SharedPtr<Counted> child0(new Counted());
    sm::SharedPtr<Counted> child1(new Counted());

    root->children.push_back(child0);
    child0->children.push_back(child1);
    root->children.push_back(child1);
}

TEST(SharedPtrTest, IntrusiveWeakShare) {
    sm::SharedPtr<Counted> root(new Counted());
    sm::SharedPtr<Counted> child0(new Counted());
    sm::SharedPtr<Counted> child1(new Counted());

    root->children.push_back(child0);
    child0->children.push_back(child1);
    root->children.push_back(child1);

    sm::WeakPtr<Counted> weak(root);
    sm::SharedPtr<Counted> shared(weak.lock());
    ASSERT_EQ(shared, root);
}

TEST(SharedPtrTest, IntrusiveWeakShareRelease) {
    sm::WeakPtr<Counted> weak;

    {
        sm::SharedPtr<Counted> root(new Counted());
        sm::SharedPtr<Counted> child0(new Counted());
        sm::SharedPtr<Counted> child1(new Counted());

        root->children.push_back(child0);
        child0->children.push_back(child1);
        root->children.push_back(child1);

        weak = root;
    }

    sm::SharedPtr<Counted> shared(weak.lock());
    ASSERT_EQ(shared, nullptr);
}
