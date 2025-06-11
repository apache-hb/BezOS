#include <gtest/gtest.h>
#include <random>

#include "std/container/btree_map.hpp"

class BTreeMapTest : public testing::Test {
public:
    using Common = sm::detail::BTreeMapCommon<int, int>;
    using LeafNode = Common::LeafNode;
    using InternalNode = Common::InternalNode;
    using Entry = Common::NodeEntry;
    using InsertResult = sm::detail::InsertResult;

    void AssertLeafFull(LeafNode& leaf) {
        ASSERT_EQ(leaf.insert({INT_MAX, INT_MAX}), InsertResult::eFull) << "Leaf node should be full after insertions";
        ASSERT_EQ(leaf.count(), leaf.capacity()) << "Leaf node should be full";
    }

    void ValidateLeafInvariants(const LeafNode& leaf) {
        auto keys = leaf.keys();
        bool sorted = std::is_sorted(keys.begin(), keys.end());
        if (!sorted) {
            printf("Leaf node %p is not sorted\n", (void*)&leaf);
            for (size_t i = 0; i < leaf.count(); i++) {
                printf("Key %zu: %d\n", i, (int)leaf.key(i));
            }
        }
        ASSERT_TRUE(sorted) << "Leaf node is not sorted";
    }

    void AssertLeafKeys(const LeafNode& leaf, std::map<int, int> expected) {
        for (size_t i = 0; i < leaf.count(); i++) {
            int key = leaf.key(i);
            ASSERT_TRUE(expected.contains(key)) << "Key " << key << " not found in expected keys";
            ASSERT_EQ(leaf.value(i), expected[key]) << "Value for key " << key << " does not match expected value";
            expected.erase(key);
        }
        ASSERT_TRUE(expected.empty()) << "Not all expected keys were found in leaf node";
    }
};

TEST_F(BTreeMapTest, Insert) {
    LeafNode leaf{nullptr};
    std::map<int, int> expectedKeys;

    for (size_t i = 0; i < leaf.capacity(); i++) {
        Entry entry = {int(i), int(i) * 10};
        expectedKeys.insert({entry.key, entry.value});
        ASSERT_EQ(leaf.insert(entry), InsertResult::eSuccess) << "Failed to insert key into leaf node";
    }

    AssertLeafFull(leaf);
    ValidateLeafInvariants(leaf);
    AssertLeafKeys(leaf, expectedKeys);
}

TEST_F(BTreeMapTest, InsertRandom) {
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, 1000);
    std::map<int, int> expectedKeys;
    LeafNode leaf{nullptr};

    while (!leaf.isFull()) {
        int key = dist(mt);
        expectedKeys.insert({key, key * 10});
        Entry entry = {key, key * 10};
        ASSERT_EQ(leaf.insert(entry), InsertResult::eSuccess) << "Failed to insert key into leaf node";
    }

    AssertLeafFull(leaf);
    ValidateLeafInvariants(leaf);
    AssertLeafKeys(leaf, expectedKeys);
}
