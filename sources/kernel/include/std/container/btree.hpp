#pragma once

#include "common/util/util.hpp"

#include "std/layout.hpp"
#include "util/memory.hpp"

#include <functional>
#include <limits.h>
#include <stdlib.h>

namespace sm {
    namespace detail {
        class TreeNodeHeader {
            static constexpr auto kLeafFlag = uint16_t(1 << ((sizeof(uint16_t) * CHAR_BIT) - 1));
            bool mIsLeaf;
            TreeNodeHeader *mParent;

        public:
            constexpr TreeNodeHeader(bool leaf, TreeNodeHeader *parent) noexcept
                : mIsLeaf(leaf)
                , mParent(parent)
            { }

            constexpr bool isLeaf() const noexcept {
                return mIsLeaf;
            }

            TreeNodeHeader *getParent() const noexcept {
                return mParent;
            }

            void setParent(TreeNodeHeader *parent) noexcept {
                mParent = parent;
            }
        };

        constexpr size_t computeNodeSize(Layout keyLayout, Layout valueLayout, size_t order) {
            // The node consists of:
            // - Node overhead: sizeof(TreeNodeHeader)
            // - Keys: sizeof(Key) * order
            // - Values: sizeof(Value) * order
            return sizeof(TreeNodeHeader)
                 + sm::roundup(keyLayout.size * order, keyLayout.align)
                 + sm::roundup(valueLayout.size * order, valueLayout.align);
        }

        constexpr size_t computeMaxOrder(Layout keyLayout, Layout valueLayout, size_t targetSize) {
            // The order is the number of keys/values per node.
            // We need to ensure that the total size of the node does not exceed the target size.
            // The node consists of:
            // - Node overhead: sizeof(TreeNodeHeader)
            // - Keys: sizeof(Key) * order
            // - Values: sizeof(Value) * order
            return (targetSize - sizeof(TreeNodeHeader)) / (keyLayout.size + valueLayout.size);
        }

        enum class InsertResult {
            eSuccess,
            eFull,
        };

        template<typename Key, typename Value>
        struct Entry {
            Key key;
            Value value;
        };

        template<typename Key, typename Value>
        class TreeNodeLeaf : public TreeNodeHeader {
            using Super = TreeNodeHeader;
        public:
            using Entry = Entry<Key, Value>;

            static constexpr size_t kOrder = std::max(3zu, computeMaxOrder(Layout::of<Key>(), Layout::of<Value>(), sm::kilobytes(4).bytes()));

        protected:
            /// @brief The number of keys and values in the node.
            uint32_t mCount = 0;

        private:
            /// @brief The keys in the node.
            Key mKeys[kOrder];

            /// @brief The values in the node.
            Value mValues[kOrder];

        protected:
            Key& key(size_t index) noexcept { return mKeys[index]; }
            const Key& key(size_t index) const noexcept { return mKeys[index]; }

            Value& value(size_t index) noexcept { return mValues[index]; }
            const Value& value(size_t index) const noexcept { return mValues[index]; }

        public:
            TreeNodeLeaf(TreeNodeHeader *parent) noexcept
                : TreeNodeLeaf(true, parent)
            { }

            TreeNodeLeaf(bool isLeaf, TreeNodeHeader *parent) noexcept
                : Super(isLeaf, parent)
            { }

            InsertResult insert(const Entry& entry) noexcept {
                if (count() >= capacity()) {
                    return InsertResult::eFull;
                }

                for (size_t i = 0; i < count(); i++) {
                    if (key(i) == entry.key) {
                        value(i) = entry.value;
                        return InsertResult::eSuccess;
                    } else if (key(i) > entry.key) {
                        // Shift the keys and values to make space for the new key.
                        for (size_t j = count(); j > i; j--) {
                            mKeys[j] = mKeys[j - 1];
                            mValues[j] = mValues[j - 1];
                        }
                        key(i) = entry.key;
                        value(i) = entry.value;
                        mCount += 1;
                        return InsertResult::eSuccess;
                    }
                }

                // If we reach here, the key is greater than all existing keys.
                key(mCount) = entry.key;
                value(mCount) = entry.value;
                mCount += 1;
                return InsertResult::eSuccess;
            }

            Value *find(const Key& k) noexcept {
                for (size_t i = 0; i < mCount; i++) {
                    if (key(i) == k) {
                        return &value(i);
                    }
                }
                return nullptr; // Key not found
            }

            const Value *find(const Key& k) const noexcept {
                for (size_t i = 0; i < mCount; i++) {
                    if (key(i) == k) {
                        return &value(i);
                    }
                }
                return nullptr; // Key not found
            }

            size_t count() const noexcept {
                return mCount;
            }

            size_t capacity() const noexcept {
                return kOrder;
            }

            void splitInto(TreeNodeLeaf *newLeaf, const Entry& entry, Entry *midpoint) noexcept {
                Entry middle = {mKeys[kOrder / 2], mValues[kOrder / 2]};
                // copy the top half of the keys and values to the new leaf
                for (size_t i = kOrder / 2; i < mCount; i++) {
                    newLeaf->key(i - kOrder / 2) = key(i);
                    newLeaf->value(i - kOrder / 2) = value(i);
                }
                newLeaf->mCount = mCount - kOrder / 2;
                mCount = kOrder / 2;

                // insert the new entry into the correct leaf
                if (entry.key < middle.key) {
                    this->insert(entry);
                } else {
                    newLeaf->insert(entry);
                }

                // set the midpoint to the middle entry
                *midpoint = middle;
            }
        };

        template<typename Key, typename Value>
        class TreeNodeInternal : public TreeNodeLeaf<Key, Value> {
            using Leaf = TreeNodeLeaf<Key, Value>;
            using Super = TreeNodeLeaf<Key, Value>;
            using Entry = Entry<Key, Value>;

        protected:
            Leaf *mChildren[Super::kOrder + 1]{};

            TreeNodeInternal(bool leaf, TreeNodeHeader *parent) noexcept
                : Super(leaf, parent)
            { }

        public:
            TreeNodeInternal(TreeNodeHeader *parent) noexcept
                : TreeNodeInternal(false, parent)
            { }

            using Super::count;
            using Super::capacity;
            using Super::key;
            using Super::value;

            Leaf *getLeaf(size_t index) const noexcept {
                return mChildren[index];
            }

            Leaf *findChild(const Key& key) const noexcept {
                for (size_t i = 0; i < Super::count(); i++) {
                    if (Super::key(i) > key) {
                        return mChildren[i];
                    }
                }

                return mChildren[Super::count()]; // Return the last child if key is greater than all keys
            }

            InsertResult rebalance(const Entry& entry, Leaf *newLeaf) noexcept {
                size_t n = count();
                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                for (size_t i = 0; i < n; i++) {
                    if (Super::key(i) == entry.key) {
                        Super::value(i) = entry.value;
                        return InsertResult::eSuccess;
                    } else if (Super::key(i) > entry.key) {
                        // Shift the keys and values to make space for the new key.
                        for (size_t j = n; j > i; j--) {
                            key(j) = Super::key(j - 1);
                            value(j) = Super::value(j - 1);
                            mChildren[j + 1] = mChildren[j];
                        }
                        key(i) = entry.key;
                        value(i) = entry.value;
                        mChildren[i + 1] = newLeaf;
                        Super::mCount += 1;
                        return InsertResult::eSuccess;
                    }
                }

                // If we reach here, the key is greater than all existing keys.
                key(n) = entry.key;
                value(n) = entry.value;
                mChildren[n + 1] = newLeaf;
                Super::mCount += 1;
                return InsertResult::eSuccess;
            }

            void initAsRoot(Leaf *lhs, Leaf *rhs, const Entry& entry) noexcept {
                mChildren[0] = lhs;
                mChildren[1] = rhs;
                key(0) = entry.key;
                value(0) = entry.value;
                Super::mCount = 1;

                lhs->setParent(this);
                rhs->setParent(this);
            }

            ~TreeNodeInternal() noexcept {
                for (size_t i = 0; i < Super::count() + 1; i++) {
                    Leaf *child = mChildren[i];
                    deleteNode<Key, Value>(child);
                }
            }
        };

        template<typename Key, typename Value>
        class TreeNodeRoot : public TreeNodeInternal<Key, Value> {
            using Super = TreeNodeInternal<Key, Value>;
            using Leaf = TreeNodeLeaf<Key, Value>;
            using Entry = Entry<Key, Value>;

        public:
            TreeNodeRoot() noexcept
                : Super(true, nullptr)
            { }

            TreeNodeRoot(Leaf *lhs, Leaf *rhs, const Entry& entry) noexcept
                : Super(false, nullptr)
            {
                Super::initAsRoot(lhs, rhs, entry);
            }
        };

        template<typename Node, typename... Args>
        Node *newNode(Args&&... args) noexcept {
            void *memory = aligned_alloc(alignof(Node), sizeof(Node));
            printf("Allocating new node at %p\n", memory);
            return new (memory) Node(std::forward<Args>(args)...);
        }

        template<typename Key, typename Value>
        void deleteNode(TreeNodeHeader *node) noexcept {
            using LeafNode = TreeNodeLeaf<Key, Value>;
            using InternalNode = TreeNodeInternal<Key, Value>;
            if (node->isLeaf()) {
                LeafNode *leaf = static_cast<LeafNode*>(node);
                std::destroy_at(leaf);
            } else {
                InternalNode *internal = static_cast<InternalNode*>(node);
                std::destroy_at(internal);
            }

            free(static_cast<void*>(node));
        }
    }

    template<
        typename Key,
        typename Value,
        typename Compare = std::less<Key>,
        typename Equal = std::equal_to<Key>
    >
    class BTreeMap {
        using Entry = detail::Entry<Key, Value>;
        using RootNode = detail::TreeNodeRoot<Key, Value>;
        using InternalNode = detail::TreeNodeInternal<Key, Value>;
        using LeafNode = detail::TreeNodeLeaf<Key, Value>;

        LeafNode *mRootNode;

        void splitNode(LeafNode *leaf, const Entry& entry) noexcept {
            Entry midpoint;

            // Splitting the root node is a special case.
            if (leaf == mRootNode) {
                InternalNode *newRoot = detail::newNode<InternalNode>(nullptr);
                LeafNode *newLeaf = leaf->isLeaf() ? detail::newNode<LeafNode>(newRoot) : detail::newNode<InternalNode>(newRoot);
                leaf->splitInto(newLeaf, entry, &midpoint);
                printf("Split root node: %p to %p with parent %p\n", leaf, newLeaf, newRoot);
                newRoot->initAsRoot(leaf, newLeaf, midpoint);
                mRootNode = newRoot;
            } else {
                InternalNode *parent = static_cast<InternalNode*>(leaf->getParent());
                LeafNode *newLeaf = leaf->isLeaf() ? detail::newNode<LeafNode>(parent) : detail::newNode<InternalNode>(parent);
                leaf->splitInto(newLeaf, entry, &midpoint);
                printf("Split node: %p to %p with parent %p\n", leaf, newLeaf, parent);
                if (leaf == mRootNode) {
                    mRootNode = newNode<RootNode>(leaf, newLeaf, midpoint);
                } else {
                    rebalance(leaf, newLeaf, midpoint);
                }
            }
        }

        void rebalance(LeafNode *node, LeafNode *newLeaf, const Entry& midpoint) noexcept {
            InternalNode *parent = static_cast<InternalNode*>(node->getParent());
            auto result = parent->rebalance(midpoint, newLeaf);
            if (result == detail::InsertResult::eFull) {
                splitNode(parent, midpoint);
            }
        }

        void insertInto(LeafNode *node, const Entry& entry) noexcept {
            if (node->isLeaf()) {
                auto result = node->insert(entry);
                if (result == detail::InsertResult::eFull) {
                    splitNode(node, entry);
                }
            } else {
                InternalNode *internalNode = static_cast<InternalNode*>(node);
                LeafNode *child = internalNode->findChild(entry.key);
                insertInto(child, entry);
            }
        }

        bool nodeContains(const LeafNode *node, const Key& key) const noexcept {
            if (node->isLeaf()) {
                return node->find(key) != nullptr;
            } else {
                const auto *internalNode = static_cast<const InternalNode*>(node);
                const LeafNode *child = internalNode->findChild(key);
                return nodeContains(child, key);
            }
        }

    public:
        constexpr BTreeMap() noexcept
            : mRootNode(nullptr)
        { }

        UTIL_NOCOPY(BTreeMap);
        UTIL_NOMOVE(BTreeMap);

        ~BTreeMap() noexcept {
            if (mRootNode) {
                detail::deleteNode<Key, Value>(mRootNode);
                mRootNode = nullptr;
            }
        }

        void insert(const Key& key, const Value& value) noexcept {
            if (mRootNode == nullptr) {
                mRootNode = detail::newNode<LeafNode>(nullptr);
            }

            insertInto(mRootNode, Entry{key, value});
        }

        bool contains(const Key& key) const noexcept {
            if (mRootNode == nullptr) {
                return false;
            }

            return nodeContains(mRootNode, key);
        }

        void validate() const noexcept {
            if (!mRootNode) {
                return; // Empty tree is valid
            }

            size_t depth = 0;
            InternalNode *current = static_cast<InternalNode*>(mRootNode);
            while (current) {
                depth++;
                if (current->isLeaf()) {
                    break; // Reached a leaf node
                }
                current = static_cast<InternalNode*>(current->getLeaf(0));
            }

            // walk the tree and ensure the depth is consistent
            auto validateNode = [&](this auto&& validateNode, const LeafNode *node, size_t currentDepth) {
                if (node->isLeaf()) {
                    if (currentDepth != depth) {
                        throw std::runtime_error("Tree depth inconsistency detected");
                    }
                    return;
                }

                const InternalNode *internalNode = static_cast<const InternalNode*>(node);

                for (size_t i = 0; i < internalNode->count(); i++) {
                    validateNode(internalNode->getLeaf(i), currentDepth + 1);
                }
                validateNode(internalNode->getLeaf(internalNode->count()), currentDepth + 1); // Last child
            };

            validateNode(mRootNode, 1);
        }
    };
}
