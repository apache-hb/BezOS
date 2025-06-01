#include <random>
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <gtest/gtest.h>

#include "std/container/btree.hpp"

using namespace sm;
using namespace sm::detail;

class BTreeTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
    }
};

TEST_F(BTreeTest, NodeInsert) {
    TreeNodeLeaf<int, int> node{nullptr};
    EXPECT_EQ(node.insert({1, 10}), InsertResult::eSuccess);
    EXPECT_EQ(*node.find(1), 10);

    EXPECT_EQ(node.insert({2, 20}), InsertResult::eSuccess);
    EXPECT_EQ(*node.find(2), 20);

    EXPECT_EQ(node.insert({1, 15}), InsertResult::eSuccess);
    EXPECT_EQ(*node.find(1), 15);

    EXPECT_EQ(node.insert({3, 30}), InsertResult::eSuccess);
    EXPECT_EQ(*node.find(3), 30);

    EXPECT_EQ(node.insert({20, 20}), InsertResult::eSuccess);
    EXPECT_EQ(node.insert({5, 50}), InsertResult::eSuccess);

    EXPECT_EQ(*node.find(20), 20);
    EXPECT_EQ(*node.find(5), 50);
    EXPECT_EQ(node.count(), 5);
}

TEST_F(BTreeTest, Insert) {
    BTreeMap<int, int> tree;
    for (size_t i = 0; i < 10000; i++) {
        tree.insert(i, i * 10);
    }

    tree.validate();
}

TEST_F(BTreeTest, InsertRandom) {
    BTreeMap<int, int> tree;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, 10000);
    for (size_t i = 0; i < 10000; i++) {
        tree.insert(dist(mt), i * 10);
    }

    tree.validate();
}

TEST_F(BTreeTest, Contains) {
    BTreeMap<int, int> tree;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, 10000);
    std::map<int, int> expected;
    for (size_t i = 0; i < 10000; i++) {
        int key = dist(mt);
        int v = i * 10;
        tree.insert(key, v);
        expected[key] = v;

        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap after insertion";
    }

    for (const auto& [key, value] : expected) {
        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap";
    }
}
