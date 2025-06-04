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

struct LeafSplitTest
    : public testing::TestWithParam<size_t> {};

INSTANTIATE_TEST_SUITE_P(
    LeafSplitMany, LeafSplitTest,
    testing::Values(0x1234, 0x5678, 0x2345, 0x3456, 0x9999));

TEST_P(LeafSplitTest, LeafSplitMany) {
    size_t seed = GetParam();

    TreeNodeLeaf<int, int> node{nullptr};
    std::mt19937 mt(seed);
    std::uniform_int_distribution<int> dist(0, 10000);

    // generate random-ish keys with no duplicates
    std::vector<int> keys;
    keys.reserve(node.capacity() + 1);
    for (size_t i = 0; i < node.capacity() + 1; i++) {
        keys.push_back(i * 10);
    }
    std::shuffle(keys.begin(), keys.end(), mt);

    for (size_t i = 0; i < node.capacity(); i++) {
        int k = keys[i];
        int v = dist(mt);
        ASSERT_EQ(node.insert({k, v}), InsertResult::eSuccess);
    }

    ASSERT_EQ(node.insert({5, 50}), InsertResult::eFull);
    ASSERT_EQ(node.count(), node.capacity());

    TreeNodeLeaf<int, int> rhs{nullptr};
    TreeNodeLeaf<int, int>::Entry midpoint;
    int key = keys[node.capacity()];
    node.splitInto(&rhs, {key, key * 10}, &midpoint);
    // neither side should contain the midpoint key
    for (size_t i = 0; i < node.count(); i++) {
        ASSERT_LT(node.key(i), midpoint.key);
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_GT(rhs.key(i), midpoint.key);
    }

    // both sides should be sorted and at least
    ASSERT_TRUE(std::is_sorted(&node.key(0), &node.key(node.count() - 1), [](const auto& a, const auto& b) {
        return a < b;
    }));

    ASSERT_TRUE(std::is_sorted(&rhs.key(0), &rhs.key(rhs.count() - 1), [](const auto& a, const auto& b) {
        return a < b;
    }));

    ASSERT_EQ(node.count(), node.capacity() / 2);
    ASSERT_EQ(rhs.count(), node.capacity() / 2);
}

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

TEST_F(BTreeTest, LeafSplit) {
    using Leaf = TreeNodeLeaf<int, int>;

    Leaf lhs{nullptr};
    Leaf rhs{nullptr};
    for (size_t i = 0; i < lhs.capacity(); i++) {
        EXPECT_EQ(lhs.insert({int(i), int(i) * 10}), InsertResult::eSuccess);
    }

    Leaf::Entry midpoint;
    lhs.splitInto(&rhs, {999, 50}, &midpoint);

    // neither side should contain the midpoint key
    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_NE(lhs.key(i), midpoint.key);
        ASSERT_LT(lhs.key(i), midpoint.key);
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_NE(rhs.key(i), midpoint.key);
        ASSERT_GT(rhs.key(i), midpoint.key);
    }
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

        // tree.validate();

        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap after insertion";
    }

    for (const auto& [key, value] : expected) {
        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap";
    }
}

TEST_F(BTreeTest, Iterate) {
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

    std::set<int> foundKeys;

    for (const auto& [key, value] : tree) {
        printf("Key: %d, Value: %d\n", key, value);
        ASSERT_FALSE(foundKeys.contains(key)) << "Key " << key << " found multiple times in BTreeMap";
        foundKeys.insert(key);

        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap";
        ASSERT_TRUE(expected.contains(key)) << "Key " << key << " not found in expected map";
        ASSERT_EQ(expected[key], value) << "Value for key " << key << " does not match expected value";
        expected.erase(key);
    }

    if (!expected.empty()) {
        printf("Expected keys not found in BTreeMap: ");
        for (const auto& [key, value] : expected) {
            printf("%d ", key);
        }
        printf("\n");
    }
    ASSERT_TRUE(expected.empty()) << "Not all expected keys were found in the BTreeMap";
}

#if 0
TEST_F(BTreeTest, Find) {
    BTreeMap<int, int> tree;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, 10000);
    std::map<int, int> expected;
    for (size_t i = 0; i < 10000; i++) {
        int key = dist(mt);
        int v = i * 10;
        tree.insert(key, v);
        expected[key] = v;

        auto it = tree.find(key);
        ASSERT_NE(it, tree.end()) << "Key " << key << " not found in BTreeMap after insertion";
        auto [foundKey, foundValue] = *it;
        ASSERT_EQ(foundKey, key) << "Found key does not match inserted key";
        ASSERT_EQ(foundValue, v) << "Found value does not match inserted value";
    }

    for (const auto& [key, value] : expected) {
        auto it = tree.find(key);
        ASSERT_NE(it, tree.end()) << "Key " << key << " not found in BTreeMap";
        auto [foundKey, foundValue] = *it;
        ASSERT_EQ(foundKey, key) << "Found key does not match expected key";
        ASSERT_EQ(foundValue, value) << "Found value does not match expected value";
    }
}
#endif