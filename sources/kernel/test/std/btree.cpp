#include <gtest/gtest.h>

#include "std/container/btree.hpp"

using namespace sm;
using namespace sm::detail;

constexpr auto kKeyLayout = sm::Layout::of<uint64_t>();
constexpr auto kValueLayout = sm::Layout::of<uint64_t>();
constexpr size_t kTargetSize = sm::kilobytes(4).bytes();

using TestNode = BTreeNode<uint64_t, uint64_t>;

TEST(BTreeNodeTest, ComputeMaxOrder) {
    constexpr size_t kMaxOrder = computeMaxOrder(kKeyLayout, kValueLayout, kTargetSize);
    EXPECT_EQ(kMaxOrder, 240);
}

TEST(BTreeNodeTest, ComputeNodeSize) {
    constexpr size_t kMaxOrder = computeMaxOrder(kKeyLayout, kValueLayout, kTargetSize);
    constexpr size_t kSize = computeNodeSize(kKeyLayout, kValueLayout, kMaxOrder);
    EXPECT_LE(kSize, kTargetSize);
}

TEST(BTreeNodeTest, ComputeNodeSizeWithAlign) {
    struct alignas(16) Key {
        uint64_t v;
    };
    constexpr auto keyLayout = sm::Layout::of<Key>();
    constexpr size_t kMaxOrder = computeMaxOrder(keyLayout, kValueLayout, kTargetSize);
    constexpr size_t kSize = computeNodeSize(keyLayout, kValueLayout, kMaxOrder);
    EXPECT_LE(kSize, kTargetSize);
}

TEST(BTreeNodeTest, FlagDataOffset) {
    constexpr size_t order = computeMaxOrder(kKeyLayout, kValueLayout, kTargetSize);
    TestNode *node = TestNode::create(order);
    EXPECT_NE(node, nullptr);
    EXPECT_EQ(node->getOrder(), order);
    EXPECT_TRUE(node->isLeaf());
    TestNode::destroy(node);
}

TEST(BTreeLayoutTest, FlagDataOffset) {
    constexpr size_t order = computeMaxOrder(kKeyLayout, kValueLayout, kTargetSize);
    constexpr auto layout = BTreeNodeLayout::of<uint64_t, uint64_t>();
    constexpr auto flagDataOffset = layout.flagDataOffset(order);
    constexpr auto flagDataSize = layout.flagDataSize(order);
    constexpr auto keyDataOffset = layout.keyDataOffset(order);
    constexpr auto keyDataSize = layout.keyDataSize(order);
    constexpr auto valueDataOffset = layout.valueDataOffset(order);
    constexpr auto valueDataSize = layout.valueDataSize(order);

    ASSERT_EQ(flagDataOffset, 0);
    ASSERT_EQ(flagDataSize, sm::roundup(order, 4zu) / 4);
    ASSERT_TRUE((keyDataOffset + sizeof(BTreeNodeHeader)) % kKeyLayout.align == 0);
    ASSERT_TRUE((valueDataOffset + sizeof(BTreeNodeHeader)) % kValueLayout.align == 0);
    ASSERT_LE((keyDataOffset + keyDataSize), valueDataOffset);
    ASSERT_LE((valueDataOffset + valueDataSize), kTargetSize);
}

TEST(BTreeNodeTest, Create) {
    TestNode *node = TestNode::create(0xFF);
    EXPECT_NE(node, nullptr);
    EXPECT_EQ(node->getOrder(), 0xFF);
    EXPECT_TRUE(node->isLeaf());
    TestNode::destroy(node);
}

#if 0
TEST(BTreeNodeTest, Insert) {
    TestNode *node = TestNode::create(0xFF);
    EXPECT_NE(node, nullptr);

    node->insert(0, 100, 200);
    EXPECT_EQ(*node->getKey(0), 100);
    EXPECT_EQ(*node->getValue(0), 200);
    EXPECT_TRUE(node->isLeaf());

    TestNode::destroy(node);
}
#endif