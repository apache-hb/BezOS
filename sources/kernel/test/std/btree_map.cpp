#include <gtest/gtest.h>
#include <random>

#include "std/container/btree_map.hpp"

class BTreeMapStringTest : public testing::Test {
    std::mt19937 mt{0x1234}; // Fixed seed for reproducibility
public:
    using Common = sm::detail::BTreeMapCommon<std::string, int>;
    using LeafNode = Common::LeafNode;
    using InternalNode = Common::InternalNode;
    using Entry = Common::NodeEntry;
    using InsertResult = sm::detail::InsertResult;

    std::string RandomString(size_t length) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<> dist(0, sizeof(charset) - 1);
        std::string str;
        str.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            str += charset[dist(mt)];
        }
        return str;
    }

    void AssertLeafFull(LeafNode& leaf) {
        ASSERT_EQ(leaf.insert({"", INT_MAX}), InsertResult::eFull) << "Leaf node should be full after insertions";
        ASSERT_EQ(leaf.count(), leaf.capacity()) << "Leaf node should be full";
    }

    void ValidateLeafInvariants(const LeafNode& leaf) {
        auto keys = leaf.keys();
        bool sorted = std::is_sorted(keys.begin(), keys.end());
        if (!sorted) {
            printf("Leaf node %p is not sorted\n", (void*)&leaf);
            for (size_t i = 0; i < leaf.count(); i++) {
                printf("Key %zu: %s\n", i, leaf.key(i).c_str());
            }
        }
        ASSERT_TRUE(sorted) << "Leaf node is not sorted";
    }

    void AssertLeafKeys(const LeafNode& leaf, std::map<std::string, int> expected) {
        for (size_t i = 0; i < leaf.count(); i++) {
            auto key = leaf.key(i);
            ASSERT_TRUE(expected.contains(key)) << "Key " << key << " not found in expected keys";
            ASSERT_EQ(leaf.value(i), expected[key]) << "Value for key " << key << " does not match expected value";
            expected.erase(key);
        }
        ASSERT_TRUE(expected.empty()) << "Not all expected keys were found in leaf node";
    }

};

TEST_F(BTreeMapStringTest, Insert) {
    LeafNode leaf{nullptr};
    std::map<std::string, int> expectedKeys;

    for (size_t i = 0; i < leaf.capacity(); i++) {
        std::string key = "key" + std::to_string(i);
        Entry entry = {key, int(i) * 10};
        expectedKeys.insert({entry.key, entry.value});
        ASSERT_EQ(leaf.insert(entry), InsertResult::eSuccess) << "Failed to insert key into leaf node";
    }

    AssertLeafFull(leaf);
    ValidateLeafInvariants(leaf);
    AssertLeafKeys(leaf, expectedKeys);
}

TEST_F(BTreeMapStringTest, InsertRandom) {
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<size_t> lengthDist(1, 10);
    std::map<std::string, int> expectedKeys;
    LeafNode leaf{nullptr};

    while (!leaf.isFull()) {
        std::string key = RandomString(lengthDist(mt));
        expectedKeys.insert({key, int(mt() % 1000)});
        Entry entry = {key, expectedKeys[key]};
        ASSERT_EQ(leaf.insert(entry), InsertResult::eSuccess) << "Failed to insert key into leaf node";
    }

    AssertLeafFull(leaf);
    ValidateLeafInvariants(leaf);
    AssertLeafKeys(leaf, expectedKeys);
}

TEST_F(BTreeMapStringTest, SplitLeafNodeHighValue) {
    LeafNode leaf{nullptr};
    std::map<std::string, int> expectedKeys;

    for (size_t i = 0; i < leaf.capacity(); i++) {
        Entry entry = {std::to_string(i), int(i) * 10};
        expectedKeys.insert({entry.key, entry.value});
        ASSERT_EQ(leaf.insert(entry), InsertResult::eSuccess) << "Failed to insert key into leaf node";
    }

    AssertLeafFull(leaf);
    ValidateLeafInvariants(leaf);
    AssertLeafKeys(leaf, expectedKeys);

    Entry newEntry = {std::to_string(leaf.capacity()), int(leaf.capacity()) * 10};
    LeafNode other{nullptr};
    leaf.splitLeafInto(&other, newEntry);
    ASSERT_EQ(leaf.count() + other.count(), leaf.capacity() + 1) << "Total count after split should equal capacity";
    ValidateLeafInvariants(leaf);
    ValidateLeafInvariants(other);
}

TEST_F(BTreeMapStringTest, SplitLeafNodeLowValue) {
    LeafNode leaf{nullptr};
    std::map<std::string, int> expectedKeys;

    for (size_t i = 0; i < leaf.capacity(); i++) {
        Entry entry = {std::to_string(i), int(i) * 10};
        expectedKeys.insert({entry.key, entry.value});
        ASSERT_EQ(leaf.insert(entry), InsertResult::eSuccess) << "Failed to insert key into leaf node";
    }

    AssertLeafFull(leaf);
    ValidateLeafInvariants(leaf);
    AssertLeafKeys(leaf, expectedKeys);

    Entry newEntry = {std::to_string(-int(leaf.capacity())), -int(leaf.capacity()) * 10};
    LeafNode other{nullptr};
    leaf.splitLeafInto(&other, newEntry);
    ASSERT_EQ(leaf.count() + other.count(), leaf.capacity() + 1) << "Total count after split should equal capacity";
    ValidateLeafInvariants(leaf);
    ValidateLeafInvariants(other);
}

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

TEST_F(BTreeMapTest, SplitLeafNodeHighValue) {
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

    ASSERT_EQ(leaf.min(), 0) << "Leaf min key should match expected";
    ASSERT_EQ(leaf.max(), leaf.count() - 1) << "Leaf max key should match expected";
    ASSERT_EQ(other.min(), leaf.count()) << "Other leaf min key should match expected";
    ASSERT_EQ(other.max(), leaf.capacity()) << "Other leaf max key should match expected";
}

TEST_F(BTreeMapTest, SplitLeafNodeLowValue) {
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

    ASSERT_EQ(leaf.min(), newEntry.key) << "Leaf min key should match expected";
    ASSERT_EQ(leaf.max(), (leaf.capacity() / 2) - 1) << "Leaf max key should match expected";
    ASSERT_EQ(other.min(), (leaf.capacity() / 2)) << "Other leaf min key should match expected";
    ASSERT_EQ(other.max(), leaf.capacity() - 1) << "Other leaf max key should match expected";
}
