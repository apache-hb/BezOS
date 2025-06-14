#include <gtest/gtest.h>
#include <random>

#include "std/container/btree_map.hpp"

struct alignas(256) BigKey {
    int value;

    BigKey(int v = 0) : value(v) {}

    constexpr auto operator<=>(const BigKey&) const noexcept = default;
    constexpr bool operator==(const BigKey& other) const noexcept {
        return value == other.value;
    }
    constexpr bool operator==(int other) const noexcept {
        return value == other;
    }

    operator int() const noexcept {
        return value;
    }
};

class BPlusTreeMapTest : public testing::Test {
public:
    using Common = sm::detail::BTreeMapCommon<BigKey, int>;
    using BaseNode = Common::Base;
    using LeafNode = Common::Leaf;
    using InternalNode = Common::Internal;
    using Entry = Common::Entry;
    using InsertResult = sm::detail::InsertResult;

    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
    }

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

TEST_F(BPlusTreeMapTest, Insert) {
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

TEST_F(BPlusTreeMapTest, InsertRandom) {
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, 100000);
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

#if 0
TEST_F(BPlusTreeMapTest, SplitLeafNodeHighValue) {
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

    Entry newEntry = {int(leaf.capacity()), int(leaf.capacity()) * 10};
    LeafNode other{nullptr};
    leaf.splitLeafInto(&other, newEntry);
    ASSERT_EQ(leaf.count() + other.count(), leaf.capacity() + 1) << "Total count after split should equal capacity";
    ValidateLeafInvariants(leaf);
    ValidateLeafInvariants(other);

    ASSERT_EQ((int)leaf.minKey(), (int)0) << "Leaf min key should match expected";
    ASSERT_EQ((int)leaf.maxKey(), (int)leaf.count() - 1) << "Leaf max key should match expected";
    ASSERT_EQ((int)other.minKey(), (int)leaf.count()) << "Other leaf min key should match expected";
    ASSERT_EQ((int)other.maxKey(), (int)leaf.capacity()) << "Other leaf max key should match expected";
}

TEST_F(BPlusTreeMapTest, SplitLeafNodeLowValue) {
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

    Entry newEntry = {-int(leaf.capacity()), -int(leaf.capacity()) * 10};
    LeafNode other{nullptr};
    leaf.splitLeafInto(&other, newEntry);
    ASSERT_EQ(leaf.count() + other.count(), leaf.capacity() + 1) << "Total count after split should equal capacity";
    ValidateLeafInvariants(leaf);
    ValidateLeafInvariants(other);

    ASSERT_EQ((int)leaf.min(),  (int)newEntry.key) << "Leaf min key should match expected";
    ASSERT_EQ((int)leaf.max(),  (int)(leaf.capacity() / 2) - 1) << "Leaf max key should match expected";
    ASSERT_EQ((int)other.min(), (int)(leaf.capacity() / 2)) << "Other leaf min key should match expected";
    ASSERT_EQ((int)other.max(), (int)leaf.capacity() - 1) << "Other leaf max key should match expected";
}
#endif
