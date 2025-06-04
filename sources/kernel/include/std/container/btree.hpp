#pragma once

#include "common/util/util.hpp"

#include "std/layout.hpp"
#include "util/memory.hpp"

#include "panic.hpp"

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

        protected:
            /// @brief The keys in the node.
            Key mKeys[kOrder];

            /// @brief The values in the node.
            Value mValues[kOrder];

            void shiftEntry(size_t index) noexcept {
                for (size_t i = count() - 1; i > index; i--) {
                    key(i) = key(i - 1);
                    value(i) = value(i - 1);
                }
            }

            Entry popBack() noexcept {
                KM_ASSERT(mCount > 0);
                Entry entry = {key(mCount - 1), value(mCount - 1)};
                mCount -= 1;
                return entry;
            }

            Entry popFront() noexcept {
                KM_ASSERT(mCount > 0);
                Entry entry = {key(0), value(0)};
                for (size_t i = 0; i < count() - 1; i++) {
                    key(i) = key(i + 1);
                    value(i) = value(i + 1);
                }
                mCount -= 1;
                return entry;
            }

            void verifyIndex(size_t index) const noexcept {
                if (index >= count()) {
                    printf("Index %zu out of bounds for node with %zu keys\n", index, count());
                    KM_PANIC("Index out of bounds in TreeNodeLeaf");
                }
            }

        public:
            void emplace(size_t index, const Entry& entry) noexcept {
                key(index) = entry.key;
                value(index) = entry.value;
            }

            Key& key(size_t index) noexcept {
                verifyIndex(index);
                return mKeys[index];
            }

            const Key& key(size_t index) const noexcept {
                verifyIndex(index);
                return mKeys[index];
            }

            Value& value(size_t index) noexcept {
                verifyIndex(index);
                return mValues[index];
            }

            const Value& value(size_t index) const noexcept {
                verifyIndex(index);
                return mValues[index];
            }

            TreeNodeLeaf(TreeNodeHeader *parent) noexcept
                : TreeNodeLeaf(true, parent)
            { }

            TreeNodeLeaf(bool isLeaf, TreeNodeHeader *parent) noexcept
                : Super(isLeaf, parent)
            { }

            InsertResult insert(const Entry& entry) noexcept {
                size_t n = count();

                for (size_t i = 0; i < n; i++) {
                    if (key(i) == entry.key) {
                        value(i) = entry.value;
                        return InsertResult::eSuccess;
                    } else if (key(i) > entry.key) {
                        if (n >= capacity()) {
                            return InsertResult::eFull;
                        }
                        // Shift the keys and values to make space for the new key.
                        mCount += 1;
                        shiftEntry(i);
                        emplace(i, entry);
                        return InsertResult::eSuccess;
                    }
                }

                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                // If we reach here, the key is greater than all existing keys.
                mCount += 1;
                emplace(n, entry);
                return InsertResult::eSuccess;
            }

            size_t upperBound(const Key& k) const noexcept {
                for (size_t i = 0; i < count(); i++) {
                    if (key(i) > k) {
                        return i;
                    }
                }
                return count();
            }

            size_t indexOf(const Key& k) const noexcept {
                for (size_t i = 0; i < mCount; i++) {
                    if (key(i) == k) {
                        return i;
                    }
                }
                return SIZE_MAX; // Key not found
            }

            Value *find(const Key& k) noexcept {
                size_t index = indexOf(k);
                if (index == SIZE_MAX) {
                    return nullptr; // Key not found
                }
                return &value(index);
            }

            const Value *find(const Key& k) const noexcept {
                size_t index = indexOf(k);
                if (index == SIZE_MAX) {
                    return nullptr; // Key not found
                }
                return &value(index);
            }

            size_t count() const noexcept {
                return mCount;
            }

            constexpr size_t capacity() const noexcept {
                return kOrder;
            }

            /// @brief Split the leaf node into two nodes to accommodate a new entry.
            ///
            /// @param newLeaf The new leaf node that will hold the keys greater than the midpoint.
            /// @param entry The entry that caused the split.
            /// @param[out] midpoint The midpoint entry that will be used to insert into the parent node.
            void splitInto(TreeNodeLeaf *newLeaf, const Entry& entry, Entry *midpoint) noexcept {
                // guardrails
                KM_ASSERT(newLeaf->count() == 0);
                KM_ASSERT(this->count() == this->capacity());
                if (this->find(entry.key) != nullptr) {
                    printf("Key %d already exists in leaf node %p %d\n", entry.key, this, this->isLeaf());
                    this->dump();
                    KM_ASSERT(this->find(entry.key) == nullptr);
                }

                size_t half = capacity() / 2;

                Entry middle = {key(half), value(half)};

                // copy the top half of the keys and values to the new leaf
                newLeaf->mCount = half;
                for (size_t i = 0; i < half; i++) {
                    newLeaf->key(i) = key(i + half);
                    newLeaf->value(i) = value(i + half);
                }
                mCount = half;

                if (entry.key < middle.key) {
                    this->insert(entry);
                    newLeaf->insert(middle);
                    *midpoint = this->popBack();
                } else {
                    newLeaf->insert(entry);
                    *midpoint = newLeaf->popFront();
                }

                if (newLeaf->count() != newLeaf->capacity() / 2) {
                    printf("New leaf node %p has %zu keys, expected at least %zu (%zu/2) %d\n",
                           newLeaf, newLeaf->count(), newLeaf->capacity() / 2, newLeaf->capacity(), midpoint->key);

                this->dump();
                newLeaf->dump();

                    KM_ASSERT(newLeaf->count() == newLeaf->capacity() / 2);
                }

                if (this->count() != this->capacity() / 2) {
                    printf("Current leaf node %p has %zu keys, expected at least %zu (%zu/2) %d\n",
                           this, this->count(), this->capacity() / 2, this->capacity(), midpoint->key);

                    this->dump();
                    newLeaf->dump();

                    KM_ASSERT(this->count() == this->capacity() / 2);
                }
            }

            void dump() {
                printf("Leaf node %p:\n", this);
                printf("  Count: %zu\n", count());
                printf("  Keys: [");
                for (size_t i = 0; i < count(); i++) {
                    printf("%d, ", key(i));
                }
                printf("]\n");
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

            Leaf *&child(size_t index) noexcept {
                KM_ASSERT(index < Super::count() + 1);
                return mChildren[index];
            }

            Leaf * const& child(size_t index) const noexcept {
                KM_ASSERT(index < Super::count() + 1);
                return mChildren[index];
            }

            Leaf *getLeaf(size_t index) const noexcept {
                return mChildren[index];
            }

            size_t findChildIndex(const Key& key) const noexcept {
                for (size_t i = 0; i < Super::count(); i++) {
                    if (Super::key(i) > key) {
                        return i;
                    }
                }

                return Super::count(); // Return the last index if key is greater than all keys
            }

            Leaf *findChild(const Key& key) const noexcept {
                return child(findChildIndex(key));
            }

            size_t indexOfChild(const Leaf *leaf) const noexcept {
                for (size_t i = 0; i < Super::count() + 1; i++) {
                    if (child(i) == leaf) {
                        return i;
                    }
                }
                return SIZE_MAX; // Not found
            }

            InsertResult rebalance(const Entry& midpoint, Leaf *newLeaf) noexcept {
                size_t n = count();
                for (size_t i = 0; i < n; i++) {
                    KM_ASSERT(Super::key(i) != midpoint.key);
                    if (Super::key(i) > midpoint.key) {
                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                        // Shift the keys and values to make space for the new key.
                        Super::mCount += 1;
                        Super::shiftEntry(i);
                        for (size_t j = n; j > i; j--) {
                            child(j + 1) = child(j);
                        }
                        Super::emplace(i, midpoint);
                        child(i + 1) = newLeaf;
                        return InsertResult::eSuccess;
                    }
                }

                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                // If we reach here, the key is greater than all existing keys.
                Super::mCount += 1;
                Super::emplace(n, midpoint);
                child(n + 1) = newLeaf;
                return InsertResult::eSuccess;
            }

            void splitInto(TreeNodeInternal *other, const Entry& entry, Entry *midpoint) noexcept {
                const size_t kHalfOrder = Super::kOrder / 2;
                const size_t kStart = kHalfOrder + 1;
                Entry middle = {Super::key(kHalfOrder), Super::value(kHalfOrder)};

                // copy the top half of the keys and values to the new internal node
                other->mCount = Super::count() - kStart;
                for (size_t i = kStart; i < Super::count(); i++) {
                    other->key(i - kStart) = Super::key(i);
                    other->value(i - kStart) = Super::value(i);
                    other->child(i - kStart) = child(i);
                }
                Super::mCount = kHalfOrder;

                // insert the new entry into the correct internal node
                if (entry.key < middle.key) {
                    this->insert(middle);
                    *midpoint = entry;
                } else {
                    other->insert(entry);
                    *midpoint = middle;
                }
            }

            void initAsRoot(Leaf *lhs, Leaf *rhs, const Entry& entry) noexcept {
                Super::mCount = 1;
                child(0) = lhs;
                child(1) = rhs;
                Super::emplace(0, entry);

                lhs->setParent(this);
                rhs->setParent(this);
            }

            ~TreeNodeInternal() noexcept {
                for (size_t i = 0; i < Super::count() + 1; i++) {
                    deleteNode<Key, Value>(child(i));
                }
            }

            void dump() {
                printf("Leaf node %p:\n", this);
                printf("  Count: %zu\n", count());
                printf("  Keys: [");
                for (size_t i = 0; i < count(); i++) {
                    printf("%d, ", key(i));
                }
                printf("]\n");
                printf("  Children: [");
                for (size_t i = 0; i < count() + 1; i++) {
                    printf("%p, ", child(i));
                }
                printf("]\n");

                for (size_t i = 0; i < count() + 1; i++) {
                    Leaf *childNode = child(i);
                    if (childNode->isLeaf()) {
                        static_cast<TreeNodeLeaf<Key, Value>*>(childNode)->dump();
                    } else {
                        static_cast<TreeNodeInternal<Key, Value>*>(childNode)->dump();
                    }
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

    struct BTreeStats {
        size_t leafCount = 0;
        size_t internalNodeCount = 0;
        size_t depth = 0;
        size_t memoryUsage = 0;
    };

    template<typename Key, typename Value>
    class BTreeMapIterator {
        using Entry = detail::Entry<Key, Value>;
        using RootNode = detail::TreeNodeRoot<Key, Value>;
        using InternalNode = detail::TreeNodeInternal<Key, Value>;
        using LeafNode = detail::TreeNodeLeaf<Key, Value>;

        LeafNode *mCurrentNode;
        size_t mCurrentIndex;

    public:
        BTreeMapIterator(LeafNode *node, size_t index) noexcept
            : mCurrentNode(node)
            , mCurrentIndex(index)
        { }

        std::pair<const Key&, Value&> operator*() noexcept {
            auto& key = mCurrentNode->key(mCurrentIndex);
            auto& value = mCurrentNode->value(mCurrentIndex);
            printf("Iterating over key %d at index %zu in node %p\n", key, mCurrentIndex, mCurrentNode);
            return {key, value};
        }

        std::pair<const Key&, const Value&> operator*() const noexcept {
            auto& key = mCurrentNode->key(mCurrentIndex);
            auto& value = mCurrentNode->value(mCurrentIndex);
            printf("Iterating over key %d at index %zu in node %p\n", key, mCurrentIndex, mCurrentNode);
            return {key, value};
        }

        bool operator==(const BTreeMapIterator& other) const noexcept {
            return mCurrentNode == other.mCurrentNode && mCurrentIndex == other.mCurrentIndex;
        }

        bool operator!=(const BTreeMapIterator& other) const noexcept {
            return mCurrentNode != other.mCurrentNode || mCurrentIndex != other.mCurrentIndex;
        }

        BTreeMapIterator& operator++() noexcept {
            if (mCurrentNode->isLeaf()) {
                if (mCurrentIndex + 1 < mCurrentNode->count()) {
                    mCurrentIndex += 1;
                } else {
                    if (InternalNode *parent = static_cast<InternalNode*>(mCurrentNode->getParent())) {
                        size_t newIndex = parent->indexOfChild(mCurrentNode);

                        printf("Exit leaf %p %zu -> %p %zu\n", mCurrentNode, mCurrentIndex, parent, newIndex);

                        mCurrentIndex = newIndex;
                        mCurrentNode = parent;
                    } else {
                        printf("Complete leaf %p %zu\n", mCurrentNode, mCurrentIndex);

                        mCurrentNode = nullptr;
                        mCurrentIndex = 0;
                    }
                }
            } else {
                if (mCurrentIndex + 1 < mCurrentNode->count()) {
                    InternalNode *internal = static_cast<InternalNode*>(mCurrentNode);
                    LeafNode *newNode = internal->child(mCurrentIndex + 1);

                    printf("Enter internal %p %zu -> %p %zu\n", mCurrentNode, mCurrentIndex + 1, newNode, 0);

                    mCurrentNode = newNode;
                    mCurrentIndex = 0;
                } else {
                    if (InternalNode *parent = static_cast<InternalNode*>(mCurrentNode->getParent())) {
                        size_t newIndex = parent->indexOfChild(mCurrentNode);

                        printf("Exit internal %p %zu -> %p %zu\n", mCurrentNode, mCurrentIndex, parent, newIndex);

                        mCurrentIndex = newIndex;
                        mCurrentNode = parent;
                    } else {
                        mCurrentNode = nullptr;
                        mCurrentIndex = 0;
                    }
                }
            }
            return *this;
        }
    };

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

        void splitRootNode(LeafNode *leaf, const Entry& entry) noexcept {
            Entry midpoint;
            InternalNode *newRoot = detail::newNode<InternalNode>(nullptr);
            LeafNode *newLeaf = leaf->isLeaf() ? detail::newNode<LeafNode>(newRoot) : detail::newNode<InternalNode>(newRoot);
            leaf->splitInto(newLeaf, entry, &midpoint);
            newRoot->initAsRoot(leaf, newLeaf, midpoint);
            mRootNode = newRoot;
        }

        void splitInnerNode(LeafNode *leaf, const Entry& entry) noexcept {
            Entry midpoint;
            bool isLeaf = leaf->isLeaf();
            InternalNode *parent = static_cast<InternalNode*>(leaf->getParent());
            LeafNode *newLeaf = isLeaf ? detail::newNode<LeafNode>(parent) : detail::newNode<InternalNode>(parent);
            if (isLeaf) {
                leaf->splitInto(newLeaf, entry, &midpoint);
            } else {
                static_cast<InternalNode*>(leaf)->splitInto(static_cast<InternalNode*>(newLeaf), entry, &midpoint);
            }

            if (leaf == mRootNode) {
                mRootNode = newNode<RootNode>(leaf, newLeaf, midpoint);
            } else {
                rebalance(leaf, newLeaf, midpoint);
            }
        }

        void splitNode(LeafNode *leaf, const Entry& entry) noexcept {
            // Splitting the root node is a special case.
            if (leaf->find(entry.key) != nullptr) {
                printf("Key %d already exists in node %p\n", entry.key, leaf);
                leaf->dump();
                KM_PANIC("Key already exists in node");
            }

            if (leaf == mRootNode) {
                splitRootNode(leaf, entry);
            } else {
                splitInnerNode(leaf, entry);
            }
        }

        void rebalance(LeafNode *node, LeafNode *newLeaf, const Entry& midpoint) noexcept {
            InternalNode *parent = static_cast<InternalNode*>(node->getParent());
            detail::InsertResult result = parent->rebalance(midpoint, newLeaf);
            if (result == detail::InsertResult::eFull) {
                splitNode(parent, midpoint);
            }
        }

        void insertInto(LeafNode *node, const Entry& entry) noexcept {
            if (node->isLeaf()) {
                detail::InsertResult result = node->insert(entry);
                if (result == detail::InsertResult::eFull) {
                    if (node->find(entry.key) != nullptr) {
                        printf("What %d already exists in node %p\n", entry.key, node);
                        node->dump();
                        KM_PANIC("Key already exists in node");
                    }
                    splitNode(node, entry);
                }
            } else {
                InternalNode *internal = static_cast<InternalNode*>(node);
                size_t index = internal->upperBound(entry.key);

                if (index != 0) {
                    if (internal->key(index - 1) == entry.key) {
                        internal->value(index - 1) = entry.value; // Update existing key
                        return;
                    }
                }

                KM_ASSERT(internal->find(entry.key) == nullptr);
                LeafNode *child = internal->child(index);
                insertInto(child, entry);
            }
        }

        bool nodeContains(const LeafNode *node, const Key& key) const noexcept {
            if (node->isLeaf()) {
                return node->find(key) != nullptr;
            } else {
                const auto *internal = static_cast<const InternalNode*>(node);
                size_t index = internal->findChildIndex(key);
                if (index != 0 && internal->key(index - 1) == key) {
                    return true; // Found the key in the internal node
                } else {
                    // Key is greater than all keys in this node, check the last child
                    LeafNode *child = internal->child(index);
                    return nodeContains(child, key);
                }
            }
        }

    public:
        using Iterator = BTreeMapIterator<Key, Value>;

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

        Iterator find(const Key& key) noexcept {
            if (mRootNode == nullptr) {
                return end();
            }

            LeafNode *current = mRootNode;
            while (current) {
                if (current->isLeaf()) {
                    size_t index = current->indexOf(key);
                    if (index != SIZE_MAX) {
                        return Iterator(current, index);
                    }
                    return end();
                } else {
                    InternalNode *internal = static_cast<InternalNode*>(current);
                    current = internal->findChild(key);
                }
            }

            return end();
        }

        Iterator begin() noexcept {
            if (mRootNode == nullptr) {
                return Iterator(nullptr, 0);
            }

            LeafNode *current = mRootNode;
            while (!current->isLeaf()) {
                current = static_cast<InternalNode*>(current)->getLeaf(0);
            }

            return Iterator(current, 0);
        }

        Iterator end() noexcept {
            return Iterator(nullptr, 0);
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

            KM_ASSERT(depth > 0);
            KM_ASSERT(current != nullptr);

            // walk the tree and ensure the depth is consistent
            auto validateNode = [&](this auto&& validateNode, const LeafNode *node, size_t currentDepth) {
                KM_ASSERT(std::is_sorted(
                    &node->key(0),
                    &node->key(node->count() - 1),
                    [compare = Compare{}](const Key& a, const Key& b) { return compare(a, b); }
                ));

                if (node != mRootNode) {
                    KM_ASSERT(node->getParent() != nullptr);
                    size_t lowerBound = node->capacity() / 2;
                    if (node->count() < lowerBound) {
                        printf("Node %p at depth %zu has %zu keys, expected at least %zu (%zu/2)\n",
                               node, currentDepth, node->count(), lowerBound, node->capacity());
                    }
                    KM_ASSERT(node->count() >= lowerBound);
                }

                if (node->isLeaf()) {
                    KM_ASSERT(currentDepth == depth);
                    return;
                }

                const InternalNode *internalNode = static_cast<const InternalNode*>(node);

                for (size_t i = 0; i < internalNode->count(); i++) {
                    LeafNode *prev = internalNode->getLeaf(i);
                    LeafNode *next = internalNode->getLeaf(i + 1);
                    for (size_t j = 0; j < prev->count(); j++) {
                        if (prev->key(j) >= internalNode->key(i)) {
                            printf("Key %d in previous node %p at depth %zu is not less than key %d in internal node %p\n",
                                   prev->key(j), prev, currentDepth, internalNode->key(i), internalNode);
                        }
                        KM_ASSERT(prev->key(j) < internalNode->key(i));
                    }

                    for (size_t j = 0; j < next->count(); j++) {
                        if (internalNode->key(i) >= next->key(j)) {
                            printf("Key %d in internal node %p at depth %zu is not less than key %d in next node %p\n",
                                   internalNode->key(i), internalNode, currentDepth, next->key(j), next);
                        }
                        KM_ASSERT(internalNode->key(i) < next->key(j));
                    }
                }

                for (size_t i = 0; i < internalNode->count() + 1; i++) {
                    validateNode(internalNode->getLeaf(i), currentDepth + 1);
                }
                validateNode(internalNode->getLeaf(internalNode->count()), currentDepth + 1); // Last child
            };

            validateNode(mRootNode, 1);
        }
    };
}
