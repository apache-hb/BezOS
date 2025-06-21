#include <gtest/gtest.h>

#include "std/container/btree.hpp"
#include <random>

using namespace sm;
using namespace sm::detail;

struct alignas(512) BigKey {
    int key;

    BigKey(int k = 0) noexcept
        : key(k)
    { }

    constexpr operator int() const noexcept {
        return key;
    }

    constexpr auto operator<=>(const BigKey&) const noexcept = default;

    constexpr bool operator==(const BigKey& other) const noexcept {
        return key == other.key;
    }

    constexpr bool operator==(int other) const noexcept {
        return key == other;
    }
};

class BTreeTest : public testing::Test {
public:
    std::mt19937 mt{0x1234};

    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
    }

    using Leaf = TreeNodeLeaf<BigKey, int>;
    using Internal = TreeNodeInternal<BigKey, int>;
    using Entry = Entry<BigKey, int>;
    using ChildEntry = typename Internal::ChildEntry;

    std::vector<int> GenerateKeys(size_t count) {
        std::vector<int> keys;
        keys.reserve(count);
        for (size_t i = 0; i < count; i++) {
            keys.push_back(int(i * 10));
        }
        return keys;
    }

    std::vector<int> GenerateRandomKeys(size_t count) {
        std::vector<int> keys = GenerateKeys(count);
        std::shuffle(keys.begin(), keys.end(), mt);
        return keys;
    }

    void AssertLeafInvariants(const Leaf& leaf) {
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

    void AssertChildNodeInvariants(const Internal& node, std::set<Leaf*>& leaves) {
        for (size_t i = 0; i < node.count() + 1; i++) {
            Leaf *child = node.child(i);
            ASSERT_NE(child, nullptr) << "Child node at index " << i << " is null in left internal node";
            ASSERT_EQ(child->getParent(), &node) << "Child node " << child << " does not have the correct parent";
            ASSERT_TRUE(leaves.contains(child)) << "Child node " << child << " not found in original leaves";
            leaves.erase(child);
        }
    }

    void DumpNode(const Leaf *node, int currentDepth) {
        for (int i = 0; i < currentDepth; i++) {
            printf("  ");
        }
        if (node == nullptr) {
            printf("Node is null at depth %d\n", currentDepth);
            return;
        }
        if (node->isLeaf()) {
            printf("%p %d %zu: [", (void*)node, currentDepth, node->count());
            for (size_t i = 0; i < node->count(); i++) {
                printf("%d, ", (int)node->key(i));
            }
            printf("]\n");
        } else {
            const Internal *internal = static_cast<const Internal*>(node);
            printf("%p %d %zu: [", (void*)internal, currentDepth, internal->count());
            for (size_t i = 0; i < internal->count(); i++) {
                printf("%d, ", (int)internal->key(i));
            }
            printf("]\n");
            for (size_t i = 0; i < internal->count() + 1; i++) {
                DumpNode(internal->getLeaf(i), currentDepth + 1);
            }
        }
    }

    int GetIndex(int x, int y, int z) {
        return (z * Leaf::maxCapacity() * (Internal::maxCapacity() + 1)) + (y * Leaf::maxCapacity()) + x;
    }
};

struct LeafSplitTest
    : public BTreeTest
    , public testing::WithParamInterface<size_t>
{ };

INSTANTIATE_TEST_SUITE_P(
    Split, LeafSplitTest,
    testing::Values(0x1234, 0x5678, 0x2345, 0x3456, 0x9999));

struct BTreeSizedTest
    : public BTreeTest
    , public testing::WithParamInterface<size_t>
{ };

INSTANTIATE_TEST_SUITE_P(
    Sized, BTreeSizedTest,
    testing::Values(1000, 1000'0));

TEST_F(BTreeTest, SplitIntoUpperHalf) {
    Internal lhs{nullptr};
    Internal rhs{nullptr};
    std::set<Leaf*> leaves;
    Leaf *firstLeaf = newNode<Leaf>(nullptr);
    Leaf *secondLeaf = newNode<Leaf>(nullptr);
    leaves.insert(firstLeaf);
    leaves.insert(secondLeaf);

    auto makeKey = [](auto i) {
        return int(i) * 5;
    };

    lhs.initAsRoot(firstLeaf, secondLeaf, {makeKey(lhs.capacity() - 1), lhs.capacity() - 1});
    for (size_t i = 0; i < lhs.capacity() - 1; i++) {
        Leaf *leaf = newNode<Leaf>(&lhs);
        ASSERT_EQ(lhs.insertChild({ makeKey(i), int(i) }, leaf), InsertResult::eSuccess) << "Failed to insert key into internal node";
        leaves.insert(leaf);
    }

    Leaf *newChild = newNode<Leaf>(&lhs);
    leaves.insert(newChild);

    Entry entry = {makeKey(lhs.capacity()), lhs.capacity()};
    Entry midpoint;
    lhs.splitIntoUpperHalf(&rhs, ChildEntry{newChild, entry}, &midpoint);

    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_LT(lhs.key(i), midpoint.key) << "Key " << lhs.key(i) << " is not less than midpoint key " << midpoint.key;
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_GT(rhs.key(i), midpoint.key) << "Key " << rhs.key(i) << " is not greater than midpoint key " << midpoint.key;
    }

    AssertChildNodeInvariants(lhs, leaves);
    AssertChildNodeInvariants(rhs, leaves);

    ASSERT_FALSE(lhs.containsInNode(midpoint.key)) << "Left node contains midpoint key after split";
    ASSERT_FALSE(rhs.containsInNode(midpoint.key)) << "Right node contains midpoint key after split";
    ASSERT_TRUE(rhs.containsInNode(entry.key)) << "Right node does not contain entry key after split";

    lhs.destroyChildren();
    rhs.destroyChildren();
}

TEST_F(BTreeTest, SplitIntoUpperHalf2) {
    Internal lhs{nullptr};
    Internal rhs{nullptr};
    std::set<Leaf*> leaves;
    Leaf *firstLeaf = newNode<Leaf>(nullptr);
    Leaf *secondLeaf = newNode<Leaf>(nullptr);
    leaves.insert(firstLeaf);
    leaves.insert(secondLeaf);

    auto makeKey = [](auto i) {
        return int(i) * 5;
    };

    lhs.initAsRoot(firstLeaf, secondLeaf, {makeKey(lhs.capacity() - 1), lhs.capacity() - 1});
    for (size_t i = 0; i < lhs.capacity() - 1; i++) {
        Leaf *leaf = newNode<Leaf>(&lhs);
        ASSERT_EQ(lhs.insertChild({ makeKey(i), int(i) }, leaf), InsertResult::eSuccess) << "Failed to insert key into internal node";
        leaves.insert(leaf);
    }

    Leaf *newChild = newNode<Leaf>(&lhs);
    leaves.insert(newChild);

    Entry entry = {makeKey(lhs.capacity() / 2) + 1, lhs.capacity()};
    Entry midpoint;
    lhs.splitIntoUpperHalf(&rhs, ChildEntry{newChild, entry}, &midpoint);

    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_LT(lhs.key(i), midpoint.key) << "Key " << lhs.key(i) << " is not less than midpoint key " << midpoint.key;
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_GT(rhs.key(i), midpoint.key) << "Key " << rhs.key(i) << " is not greater than midpoint key " << midpoint.key;
    }

    AssertChildNodeInvariants(lhs, leaves);
    AssertChildNodeInvariants(rhs, leaves);

    ASSERT_FALSE(lhs.containsInNode(midpoint.key)) << "Left node contains midpoint key after split";
    ASSERT_FALSE(rhs.containsInNode(midpoint.key)) << "Right node contains midpoint key after split";
    ASSERT_TRUE(rhs.containsInNode(entry.key)) << "Right node does not contain entry key after split";


    lhs.destroyChildren();
    rhs.destroyChildren();
}

TEST_F(BTreeTest, SplitIntoLowerHalf) {
    Internal lhs{nullptr};
    Internal rhs{nullptr};
    std::set<Leaf*> leaves;
    Leaf *firstLeaf = newNode<Leaf>(nullptr);
    Leaf *secondLeaf = newNode<Leaf>(nullptr);
    leaves.insert(firstLeaf);
    leaves.insert(secondLeaf);

    auto makeKey = [](auto i) {
        return int(i) * 5;
    };

    lhs.initAsRoot(firstLeaf, secondLeaf, {makeKey(lhs.capacity() - 1), lhs.capacity() - 1});
    for (size_t i = 0; i < lhs.capacity() - 1; i++) {
        Leaf *leaf = newNode<Leaf>(&lhs);
        ASSERT_EQ(lhs.insertChild({ makeKey(i), int(i) }, leaf), InsertResult::eSuccess) << "Failed to insert key into internal node";
        leaves.insert(leaf);
    }

    Leaf *newChild = newNode<Leaf>(&lhs);
    leaves.insert(newChild);

    Entry entry = {3, lhs.capacity()};
    Entry midpoint;
    lhs.splitIntoLowerHalf(&rhs, ChildEntry{newChild, entry}, &midpoint);

    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_LT(lhs.key(i), midpoint.key) << "Key " << lhs.key(i) << " is not less than midpoint key " << midpoint.key;
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_GT(rhs.key(i), midpoint.key) << "Key " << rhs.key(i) << " is not greater than midpoint key " << midpoint.key;
    }

    AssertChildNodeInvariants(lhs, leaves);
    AssertChildNodeInvariants(rhs, leaves);

    ASSERT_FALSE(lhs.containsInNode(midpoint.key)) << "Left node contains midpoint key after split";
    ASSERT_FALSE(rhs.containsInNode(midpoint.key)) << "Right node contains midpoint key after split";
    ASSERT_TRUE(lhs.containsInNode(entry.key)) << "Right node does not contain entry key after split";

    lhs.destroyChildren();
    rhs.destroyChildren();
}

TEST_F(BTreeTest, MergeLeafNodes) {
    Internal internal{nullptr};

    Leaf *firstLeaf = newNode<Leaf>(nullptr);
    Leaf *secondLeaf = newNode<Leaf>(nullptr);

    std::set<int> expected;

    int v = 0;
    for (size_t i = 0; i < Leaf::minCapacity() - 1; i++) {
        v += 1;
        firstLeaf->insert({ v, v * 10 });

        expected.insert(v);
    }

    v += 1;
    int rootMiddle = v;
    expected.insert(rootMiddle);

    for (size_t i = 0; i < Leaf::minCapacity() - 1; i++) {
        v += 1;
        secondLeaf->insert({ v, v * 10 });
        expected.insert(v);
    }

    internal.initAsRoot(firstLeaf, secondLeaf, { rootMiddle, rootMiddle * 10 });

    for (size_t i = internal.count(); i < internal.capacity(); i++) {
        Leaf *leaf = newNode<Leaf>(&internal);
        int middle = v + 1;

        v += 1;

        for (size_t j = 0; j < Leaf::minCapacity() - 1; j++) {
            v += 1;
            leaf->insert({ v, v * 10 });
            expected.insert(v);
        }

        internal.insertChild({ middle, middle * 10 }, leaf);
        expected.insert(middle);
    }

    Leaf *lhs = internal.child(Internal::maxCapacity() / 2);
    Leaf *rhs = internal.child(Internal::maxCapacity() / 2 + 1);
    ASSERT_TRUE(lhs->isUnderFilled()) << "Left leaf is not underfilled: " << lhs->count();
    ASSERT_TRUE(rhs->isUnderFilled()) << "Right leaf is not underfilled: " << rhs->count();

    internal.mergeLeafNodes(lhs, rhs);

    internal.validate();

    ASSERT_EQ(internal.count(), internal.capacity() - 1) << "Internal leaf not deleted";

    deleteNode<BigKey, int>(rhs);

    internal.destroyChildren();
}

TEST_P(LeafSplitTest, SplitLeaf) {
    Leaf lhs{nullptr};
    Leaf rhs{nullptr};

    mt.seed(GetParam());
    std::uniform_int_distribution<int> dist(0, 10000);

    // generate random-ish keys with no duplicates
    std::set<int> expected;
    size_t size = lhs.capacity() + 1;
    std::vector<int> keys = GenerateRandomKeys(size);
    std::copy(keys.begin(), keys.end(), std::inserter(expected, expected.end()));

    for (size_t i = 0; i < lhs.capacity(); i++) {
        int k = keys[i];
        int v = dist(mt);
        ASSERT_EQ(lhs.insert({k, v}), InsertResult::eSuccess);
    }

    ASSERT_EQ(lhs.insert({5, 50}), InsertResult::eFull);
    ASSERT_EQ(lhs.count(), lhs.capacity());

    Entry midpoint;
    int key = keys[lhs.capacity()];
    lhs.splitInto(&rhs, {key, key * 10}, &midpoint);

    ASSERT_TRUE(expected.contains(midpoint.key))
        << "Midpoint key " << midpoint.key << " not found in original keys";
    expected.erase(midpoint.key);

    // neither side should contain the midpoint key
    for (size_t i = 0; i < lhs.count(); i++) {
        if (!(lhs.key(i) < midpoint.key)) {
            printf("Midpoint key: %d\n", (int)midpoint.key);
            lhs.dump(0);
        }
        ASSERT_LT(lhs.key(i), midpoint.key);

        ASSERT_TRUE(expected.contains(lhs.key(i)))
            << "Key " << lhs.key(i) << " not found in original keys";

        expected.erase(lhs.key(i));
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        if (!(rhs.key(i) > midpoint.key)) {
            printf("Midpoint key: %d\n", (int)midpoint.key);
            rhs.dump(0);
        }
        ASSERT_GT(rhs.key(i), midpoint.key);

        ASSERT_TRUE(expected.contains(rhs.key(i)))
            << "Key " << rhs.key(i) << " not found in original keys";

        expected.erase(rhs.key(i));
    }

    if (!expected.empty()) {
        printf("Keys not found in split: ");
        for (const auto& key : expected) {
            printf("%d ", key);
        }
        printf("\n");
    }
    ASSERT_TRUE(expected.empty())
        << "Not all keys were found in node after split";

    // both sides should be sorted
    AssertLeafInvariants(lhs);
    AssertLeafInvariants(rhs);

    ASSERT_TRUE(lhs.count() >= (lhs.capacity() / 2) - 1)
        << "Left node has too few keys after split: " << lhs.count();
    ASSERT_TRUE(rhs.count() >= (rhs.capacity() / 2) - 1)
        << "Right node has too few keys after split: " << rhs.count();
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

    std::set<int> keys;
    Leaf lhs{nullptr};
    Leaf rhs{nullptr};
    for (size_t i = 0; i < lhs.capacity(); i++) {
        EXPECT_EQ(lhs.insert({int(i), int(i) * 10}), InsertResult::eSuccess);
        keys.insert(int(i));
    }

    Leaf::Entry midpoint;
    lhs.splitInto(&rhs, {999, 50}, &midpoint);
    keys.insert(999);

    // neither side should contain the midpoint key
    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_NE(lhs.key(i), midpoint.key);
        ASSERT_LT(lhs.key(i), midpoint.key);
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_NE(rhs.key(i), midpoint.key);
        ASSERT_GT(rhs.key(i), midpoint.key);
    }

    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_TRUE(keys.contains(lhs.key(i))) << "Key " << lhs.key(i) << " not found in original keys";
        keys.erase(lhs.key(i));
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_TRUE(keys.contains(rhs.key(i))) << "Key " << rhs.key(i) << " not found in original keys";
        keys.erase(rhs.key(i));
    }

    keys.erase(midpoint.key);

    ASSERT_TRUE(keys.empty()) << "Not all keys were found in node after split";
}

TEST_P(LeafSplitTest, InternalSplitMany) {
    Internal lhs{nullptr};
    Internal rhs{nullptr};

    mt.seed(GetParam());
    std::vector<int> kvec = GenerateRandomKeys(lhs.capacity() + 1);
    std::set<Leaf*> leaves;

    Leaf *firstLeaf = newNode<Leaf>(nullptr);
    Leaf *secondLeaf = newNode<Leaf>(nullptr);
    leaves.insert(firstLeaf);
    leaves.insert(secondLeaf);
    lhs.initAsRoot(firstLeaf, secondLeaf, {kvec[0], kvec[0] * 10});
    std::set<int> keys;
    keys.insert(kvec[0]);
    for (size_t i = 0; i < lhs.capacity() - 1; i++) {
        Leaf *leaf = newNode<Leaf>(&lhs);
        Entry entry = { kvec[i + 1], kvec[i + 1] * 10 };
        leaf->insert(entry);
        ASSERT_EQ(lhs.insertChild(entry, leaf), InsertResult::eSuccess) << "Failed to insert key " << entry.key << " into internal node";
        leaves.insert(leaf);
        keys.insert(entry.key);
    }

    Leaf *tmpLeaf = newNode<Leaf>(&lhs);
    ASSERT_EQ(lhs.insertChild({ 930, 50 }, tmpLeaf), InsertResult::eFull);
    deleteNode<int, int>(tmpLeaf);

    Leaf::Entry midpoint;
    Entry entry = {kvec[lhs.capacity()], kvec[lhs.capacity()] * 10};
    Leaf *newChild = newNode<Leaf>(&lhs);
    newChild->insert({kvec[lhs.capacity()] + 1, kvec[lhs.capacity()] * 10});
    lhs.splitInternalNode(&rhs, ChildEntry{newChild, entry}, &midpoint);
    keys.insert(entry.key);
    leaves.insert(newChild);

    // neither side should contain the midpoint key
    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_NE(lhs.key(i), midpoint.key);
        ASSERT_LT(lhs.key(i), midpoint.key);
    }

    for (size_t i = 0; i < lhs.count() + 1; i++) {
        Leaf* child = lhs.child(i);
        ASSERT_NE(child, nullptr);
        ASSERT_EQ(child->getParent(), &lhs);
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_NE(rhs.key(i), midpoint.key);
        ASSERT_GT(rhs.key(i), midpoint.key);
    }

    AssertChildNodeInvariants(lhs, leaves);
    AssertChildNodeInvariants(rhs, leaves);

    for (Leaf *leaf : leaves) {
        printf("Leaf node %p not found in either side after split\n", (void*)leaf);
    }
    ASSERT_TRUE(leaves.empty()) << "Not all leaves were found in node after split";


    lhs.destroyChildren();
    rhs.destroyChildren();
}

TEST_P(LeafSplitTest, SplitInternalNodeMany) {
    Internal lhs{nullptr};
    Internal rhs{nullptr};

    mt.seed(GetParam());
    std::vector<int> kvec = GenerateRandomKeys(lhs.capacity() + 1);

    lhs.initAsRoot(newNode<Leaf>(nullptr), newNode<Leaf>(nullptr), {kvec[0], kvec[0] * 10});
    std::set<int> keys;
    keys.insert(kvec[0]);
    for (size_t i = 0; i < lhs.capacity() - 1; i++) {
        Leaf *leaf = newNode<Leaf>(&lhs);
        Entry entry = { kvec[i + 1], kvec[i + 1] * 10 };
        leaf->insert(entry);
        ASSERT_EQ(lhs.insertChild(entry, leaf), InsertResult::eSuccess) << "Failed to insert key " << entry.key << " into internal node";
        keys.insert(entry.key);
    }

    Leaf *tmpLeaf = newNode<Leaf>(&lhs);
    ASSERT_EQ(lhs.insertChild({ 930, 50 }, tmpLeaf), InsertResult::eFull);
    deleteNode<int, int>(tmpLeaf);

    Leaf::Entry midpoint;
    Leaf *newChild = newNode<Leaf>(&lhs);
    newChild->insert({kvec[lhs.capacity()] + 1, kvec[lhs.capacity()] * 10});
    Entry entry = {kvec[lhs.capacity()], kvec[lhs.capacity()] * 10};
    lhs.splitInternalNode(&rhs, ChildEntry{newChild, entry}, &midpoint);
    keys.insert(entry.key);

    // neither side should contain the midpoint key
    for (size_t i = 0; i < lhs.count(); i++) {
        ASSERT_NE(lhs.key(i), midpoint.key);
        ASSERT_LT(lhs.key(i), midpoint.key);
    }

    bool found = false;
    if (entry.key == midpoint.key) {
        found = true;
    } else if (entry.key > midpoint.key) {
        for (size_t i = 0; i < rhs.count(); i++) {
            if (rhs.key(i) == entry.key) {
                found = true;
                break;
            }
        }
    } else {
        for (size_t i = 0; i < lhs.count(); i++) {
            if (lhs.key(i) == entry.key) {
                found = true;
                break;
            }
        }
    }

    ASSERT_TRUE(found) << "Entry key " << entry.key << " not found in either side after split";

    for (size_t i = 0; i < lhs.count() + 1; i++) {
        Leaf* child = lhs.child(i);
        ASSERT_NE(child, nullptr);
        ASSERT_EQ(child->getParent(), &lhs);
    }

    for (size_t i = 0; i < rhs.count(); i++) {
        ASSERT_NE(rhs.key(i), midpoint.key);
        ASSERT_GT(rhs.key(i), midpoint.key);
    }

    for (size_t i = 0; i < rhs.count() + 1; i++) {
        Leaf* child = rhs.child(i);
        ASSERT_NE(child, nullptr);
        ASSERT_EQ(child->getParent(), &rhs);
    }

    lhs.destroyChildren();
    rhs.destroyChildren();
}

TEST_P(BTreeSizedTest, Iterate) {
    size_t count = GetParam();
    BTreeMap<BigKey, int> tree;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, count);
    std::map<int, int> expected;
    for (size_t i = 0; i < count; i++) {
        int key = dist(mt);
        int v = i * 10;
        tree.insert(key, v);
        expected[key] = v;

        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap after insertion";
    }

    std::set<int> foundKeys;

    // tree.dump();

    for (const auto& [key, value] : tree) {
        ASSERT_FALSE(foundKeys.contains(key)) << "Key " << key << " found multiple times in BTreeMap";
        foundKeys.insert(key);

        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap";
        ASSERT_TRUE(expected.contains(key)) << "Key " << key << " not found in expected map";
        ASSERT_EQ(expected[key], value) << "Value for key " << key << " does not match expected value";
        expected.erase(key);
    }

    if (!expected.empty()) {
        tree.dump();
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

TEST_P(BTreeSizedTest, Contains) {
    size_t count = GetParam();
    BTreeMap<BigKey, int> tree;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, 0xFFFFFFF);
    std::map<BigKey, int> expected;
    for (size_t i = 0; i < count; i++) {
        int key = dist(mt);
        int v = i * 10;
        tree.insert(key, v);
        expected[key] = v;

        if (!tree.contains(key)) {
            tree.dump();
        }

        ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap after insertion";
    }

    for (const auto& [key, value] : expected) {
        if (!tree.contains(key)) {
            tree.dump();
        }
        ASSERT_TRUE(tree.contains(key)) << "Key " << key.key << " not found in BTreeMap";
    }
}

TEST_P(BTreeSizedTest, Find) {
    BTreeMap<BigKey, int> tree;
    std::mt19937 mt(0x1234);

    size_t count = GetParam();
    std::uniform_int_distribution<int> dist(0, count);
    std::map<int, int> expected;
    for (size_t i = 0; i < count; i++) {
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

TEST_F(BTreeTest, Erase) {
    BTreeMap<BigKey, int> tree;
    std::uniform_int_distribution<int> dist(0, 1000000);
    std::map<int, int> expected;

    for (size_t i = 0; i < 1000; i++) {
        int key = dist(mt);
        int v = i * 10;
        tree.insert(key, v);
        expected[key] = v;
    }

    auto takeRandomKey = [&] {
        auto it = expected.begin();
        std::advance(it, dist(mt) % expected.size());
        auto key = it->first;
        expected.erase(it);
        return key;
    };

    for (size_t i = 0; i < 400; i++) {
        auto key = takeRandomKey();
        tree.remove(key);
        ASSERT_FALSE(tree.contains(key)) << "Key " << key << " found in BTreeMap after removal";
        ASSERT_TRUE(expected.find(key) == expected.end()) << "Key " << key << " found in expected map after removal";
    }
}

TEST_F(BTreeTest, Stats) {
    BTreeMap<BigKey, int> tree;
    std::uniform_int_distribution<int> dist(0, 1000000);
    std::map<int, int> expected;

    for (size_t i = 0; i < 1000; i++) {
        int key = dist(mt);
        int v = i * 10;
        tree.insert(key, v);
        expected[key] = v;
    }

    ASSERT_EQ(tree.count(), expected.size()) << "BTreeMap count does not match expected size";

    auto stats = tree.stats();
    ASSERT_GT(stats.leafCount, 0) << "Leaf count should be greater than 0";
    ASSERT_GT(stats.internalNodeCount, 0) << "Internal node count should be greater than 0";
    ASSERT_GT(stats.memoryUsage, 0) << "Key count should be greater than 0";
    ASSERT_GT(stats.depth, 0) << "Depth should be greater than 0";
}

TEST_F(BTreeTest, MergeInternal) {
    Internal node{nullptr};

    Internal *i0 = newNode<Internal>(&node);
    Internal *i1 = newNode<Internal>(&node);
    Internal *i2 = newNode<Internal>(&node);

    {
        Leaf *l0 = newNode<Leaf>(i0);
        l0->insert({10, 10});
        l0->insert({20, 20});
        l0->insert({30, 30});

        Leaf *l1 = newNode<Leaf>(i0);
        l1->insert({40, 40});
        l1->insert({50, 50});
        l1->insert({60, 60});

        i0->initAsRoot(l0, l1, {35, 30});
    }

    {
        Leaf *l0 = newNode<Leaf>(i1);
        l0->insert({70, 70});
        l0->insert({80, 80});
        l0->insert({90, 90});

        Leaf *l1 = newNode<Leaf>(i1);
        l1->insert({100, 100});
        l1->insert({110, 110});
        l1->insert({120, 120});

        i1->initAsRoot(l0, l1, {95, 950});
    }

    {
        Leaf *l0 = newNode<Leaf>(i2);
        l0->insert({130, 130});
        l0->insert({140, 140});
        l0->insert({150, 150});

        Leaf *l1 = newNode<Leaf>(i2);
        l1->insert({160, 160});
        l1->insert({170, 170});
        l1->insert({180, 180});

        i2->initAsRoot(l0, l1, {155, 1550});
    }

    node.initAsRoot(i0, i1, {65, 750});
    node.insertChild({125, 1250}, i2);

    node.dump(0);

    node.mergeInternalNodes(i0, i1);

    node.dump(0);

    node.destroyChildren();
}

#if 0
TEST_F(BTreeTest, EraseAll) {
    BTreeMap<BigKey, int> tree;
    std::uniform_int_distribution<int> dist(0, 1000000);
    std::map<int, int> expected;

    for (size_t i = 0; i < 1000; i++) {
        int key = dist(mt);
        int v = i * 10;
        tree.insert(key, v);
        expected[key] = v;
    }

    auto takeRandomKey = [&] {
        auto it = expected.begin();
        std::advance(it, dist(mt) % expected.size());
        auto key = it->first;
        expected.erase(it);
        return key;
    };

    while (!expected.empty()) {
        auto key = takeRandomKey();
        tree.remove(key);
        tree.validate();
        ASSERT_FALSE(tree.contains(key)) << "Key " << key << " found in BTreeMap after removal";
        ASSERT_TRUE(expected.find(key) == expected.end()) << "Key " << key << " found in expected map after removal";
    }
}
#endif
