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

            bool isRootNode() const noexcept {
                return getParent() == nullptr;
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

        constexpr size_t computeMaxLeafOrder(Layout keyLayout, Layout valueLayout, size_t targetSize) {
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

            static constexpr size_t kOrder = std::max(3zu, computeMaxLeafOrder(Layout::of<Key>(), Layout::of<Value>(), sm::kilobytes(4).bytes()));

        public:
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
                    printf("Index %zu out of bounds for node %p with %zu keys\n", index, this, count());
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

            void remove(size_t index) noexcept {
                verifyIndex(index);
                for (size_t i = index; i < count(); i++) {
                    key(i) = key(i + 1);
                    value(i) = value(i + 1);
                }
                mCount -= 1;
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

            bool isFull() const noexcept {
                return count() == capacity();
            }

            bool isUnderFilled() const noexcept {
                return count() < leastCount();
            }

            constexpr size_t leastCount() const noexcept {
                return (capacity() / 2) - 1;
            }

            constexpr size_t capacity() const noexcept {
                return kOrder;
            }

            const Key& minKey() const noexcept {
                KM_ASSERT(count() > 0);
                return key(0);
            }

            const Key& maxKey() const noexcept {
                KM_ASSERT(count() > 0);
                return key(count() - 1);
            }

            std::span<const Key> keys() const noexcept {
                return std::span<const Key>(mKeys, count());
            }

            std::span<Key> keys() noexcept {
                return std::span<Key>(mKeys, count());
            }

            /// @brief Split the internal node into two nodes to accommodate a new entry and leaf.
            void splitInto(TreeNodeLeaf *other, const Entry& entry, Entry *midpoint) noexcept {
                // guardrails
                KM_ASSERT(other->count() == 0);
                KM_ASSERT(this->count() == this->capacity());
                if (this->find(entry.key) != nullptr) {
                    printf("Key %d already exists in leaf node %p %d\n", (int)entry.key, this, this->isLeaf());
                    this->dump();
                    KM_ASSERT(this->find(entry.key) == nullptr);
                }

                bool isOdd = (capacity() & 1);
                size_t half = capacity() / 2;
                size_t otherSize = half + (isOdd ? 1 : 0);

                Entry middle = {key(half), value(half)};

                // copy the top half of the keys and values to the new leaf
                other->mCount = otherSize;
                for (size_t i = 0; i < otherSize; i++) {
                    other->key(i) = key(i + half);
                    other->value(i) = value(i + half);
                }
                mCount = half;

                if (entry.key < middle.key) {
                    this->insert(entry);
                    other->insert(middle);
                    *midpoint = this->popBack();
                } else {
                    other->insert(entry);
                    *midpoint = other->popFront();
                }

                if (other->isUnderFilled()) {
                    printf("New leaf node %p has %zu keys, expected at least %zu (%zu/2) %d\n",
                           other, other->count(), other->capacity() / 2, other->capacity(), (int)midpoint->key);

                    this->dump();
                    other->dump();

                    KM_ASSERT(!other->isUnderFilled());
                }

                if (this->isUnderFilled()) {
                    printf("Current leaf node %p has %zu keys, expected at least %zu (%zu/2) %d\n",
                           this, this->count(), this->capacity() / 2, this->capacity(), (int)midpoint->key);

                    this->dump();
                    other->dump();

                    KM_ASSERT(!this->isUnderFilled());
                }
            }

            bool containsInNode(const Key& key) const noexcept {
                for (size_t i = 0; i < count(); i++) {
                    if (this->key(i) == key) {
                        return true;
                    }
                }
                return false;
            }

            void dump() const {
                printf("Leaf node %p:\n", this);
                printf("  IsLeaf: %d\n", Super::isLeaf());
                printf("  Count: %zu\n", count());
                printf("  Keys: [");
                for (size_t i = 0; i < count(); i++) {
                    printf("%d, ", (int)key(i));
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
            using Super::mCount;
            using Super::emplace;
            using Super::minKey;
            using Super::maxKey;
            using Super::upperBound;

            struct ChildEntry {
                Leaf *child;
                Entry entry;
            };

            ChildEntry popBackChild() noexcept {
                KM_ASSERT(count() > 0);
                Entry entry = {key(count() - 1), value(count() - 1)};
                Leaf *leaf = child(count() - 1);
                mCount -= 1;
                return ChildEntry{leaf, entry};
            }

            /// @brief Returns the first element and the child node before it.
            ChildEntry popFrontChild() noexcept {
                KM_ASSERT(count() > 0);
                Entry entry = {key(0), value(0)};
                Leaf *leaf = child(0);
                for (size_t i = 0; i < count() - 1; i++) {
                    key(i) = key(i + 1);
                    value(i) = value(i + 1);
                }

                for (size_t i = 0; i < count(); i++) {
                    child(i) = child(i + 1);
                }

                mCount -= 1;
                return entry;
            }

            Leaf *&child(size_t index) noexcept {
                KM_ASSERT(index < Super::count() + 1);
                return mChildren[index];
            }

            Leaf * const& child(size_t index) const noexcept {
                KM_ASSERT(index < Super::count() + 1);
                return mChildren[index];
            }

            void takeChild(size_t index, Leaf *leaf) noexcept {
                leaf->setParent(this);
                child(index) = leaf;
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
                for (size_t i = 0; i < count() + 1; i++) {
                    if (child(i) == leaf) {
                        return i;
                    }
                }
                return SIZE_MAX; // Not found
            }

            bool containsLeafInNode(const Leaf *leaf) const noexcept {
                for (size_t i = 0; i < count() + 1; i++) {
                    if (child(i) == leaf) {
                        return true;
                    }
                }
                return false;
            }

            void removeEntry(size_t index) noexcept {
                KM_ASSERT(index < count());
                for (size_t i = index; i < count() - 1; i++) {
                    key(i) = key(i + 1);
                    value(i) = value(i + 1);
                }

                for (size_t i = index; i < count(); i++) {
                    child(i) = child(i + 1);
                }

                mCount -= 1;
            }

            Entry promoteFront() noexcept {
                Entry entry = {key(0), value(0)};
                Leaf *leaf = child(0);
                if (leaf->isLeaf()) {
                    Entry promoted = leaf->popFront();
                    key(0) = promoted.key;
                    value(0) = promoted.value;
                    return entry;
                } else {
                    TreeNodeInternal *internal = static_cast<TreeNodeInternal*>(leaf);
                    Entry promoted = internal->promoteFront();
                    key(0) = promoted.key;
                    value(0) = promoted.value;
                    return entry;
                }
            }

            Entry promoteBack() noexcept {
                Entry entry = {key(count() - 1), value(count() - 1)};
                Leaf *leaf = child(count());
                if (leaf->isLeaf()) {
                    Entry promoted = leaf->popFront();
                    key(count() - 1) = promoted.key;
                    value(count() - 1) = promoted.value;
                    return entry;
                } else {
                    TreeNodeInternal *internal = static_cast<TreeNodeInternal*>(leaf);
                    Entry promoted = internal->promoteFront();
                    key(count() - 1) = promoted.key;
                    value(count() - 1) = promoted.value;
                    return entry;
                }
            }

            void removeAndPromoteChild(size_t index) noexcept {
                Leaf *lhsChild = child(index);
                Leaf *rhsChild = child(index + 1);
                size_t lhsSize = lhsChild->count();
                size_t rhsSize = rhsChild->count();
                if (lhsSize > rhsSize) {
                    // Promote from the left child
                    if (lhsChild->isLeaf()) {
                        Entry promoted = lhsChild->popBack();
                        emplace(index, promoted);
                    } else {
                        TreeNodeInternal *lhsInternal = static_cast<TreeNodeInternal*>(lhsChild);
                        Entry promoted = lhsInternal->promoteBack();
                        emplace(index, promoted);
                    }
                } else {
                    // Promote from the right child
                    if (rhsChild->isLeaf()) {
                        Entry promoted = rhsChild->popFront();
                        emplace(index, promoted);
                    } else {
                        TreeNodeInternal *rhsInternal = static_cast<TreeNodeInternal*>(rhsChild);
                        Entry promoted = rhsInternal->promoteFront();
                        emplace(index, promoted);
                    }
                }
            }

            /// @brief Insert a new entry and the leaf node above it into the internal node.
            InsertResult insertChild(const Entry& entry, Leaf *leaf) noexcept {
                size_t n = count();

                for (size_t i = 0; i < n; i++) {
                    if (key(i) == entry.key) {
                        printf("Key %d already exists in internal node %p at %zu\n", (int)entry.key, this, i);
                        dump();
                        KM_ASSERT(key(i) != entry.key);
                    }

                    if (key(i) > entry.key) {
                        if (n >= capacity()) {
                            return InsertResult::eFull;
                        }

                        // Shift the keys and values to make space for the new key.
                        mCount += 1;
                        Super::shiftEntry(i);
                        for (size_t j = n; j > i; j--) {
                            child(j + 1) = child(j);
                        }
                        emplace(i, entry);
                        leaf->setParent(this);
                        child(i + 1) = leaf;
                        return InsertResult::eSuccess;
                    }
                }

                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                // If we reach here, the key is greater than all existing keys.
                mCount += 1;
                emplace(n, entry);
                leaf->setParent(this);
                child(n + 1) = leaf;
                return InsertResult::eSuccess;
            }

            /// @brief Split the internal node into two nodes to accommodate a new entry.
            void splitInto(TreeNodeInternal *other, const Entry& entry, Entry *midpoint) noexcept {
                // guardrails
                KM_ASSERT(other->count() == 0);
                KM_ASSERT(this->count() == this->capacity());
                for (size_t i = 0; i < Super::count(); i++) {
                    if (key(i) == entry.key) {
                        printf("Key %d already exists in internal node %p %d\n", (int)entry.key, this, this->isLeaf());
                        this->dump();
                        KM_PANIC("Key already exists in internal node");
                    }
                }
                KM_ASSERT(!this->isLeaf());
                KM_ASSERT(!other->isLeaf());

                for (size_t i = 0; i < count(); i++) {
                    if (child(i) == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i, this);
                        this->dump();
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                }

                const size_t kHalfOrder = Super::kOrder / 2;

                Entry middle = {key(kHalfOrder), value(kHalfOrder)};
                Leaf *middleLeaf = child(kHalfOrder + 1);

                // copy the top half of the keys and values to the new internal node
                other->mCount = kHalfOrder - 1;
                for (size_t i = 0; i < kHalfOrder - 1; i++) {
                    other->key(i) = key(i + kHalfOrder + 1);
                    other->value(i) = value(i + kHalfOrder + 1);
                }

                for (size_t i = 0; i < kHalfOrder; i++) {
                    Leaf *childNode = child(i + kHalfOrder + 1);
                    if (childNode == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i + kHalfOrder, this);
                        this->dump();
                        KM_ASSERT(childNode != nullptr);
                    }
                    childNode->setParent(other);
                    other->child(i) = childNode;
                }

                mCount = kHalfOrder;

                if (entry.key < key(count() - 1)) {
                    this->insert(entry);
                    *midpoint = this->popBack();
                } else if (entry.key < middle.key) {
                    other->insert(middle);
                    *midpoint = entry;
                } else {
                    *midpoint = middle;
                }

                KM_ASSERT(this->count() >= (this->capacity() / 2) - 1);
                KM_ASSERT(other->count() >= (other->capacity() / 2) - 1);
            }

            void transferInto(TreeNodeInternal *other, Entry *midpoint) noexcept {
                size_t half = capacity() / 2;
                Entry middle = {key(half), value(half)};

                other->mCount = half;
                for (size_t i = 0; i < half; i++) {
                    other->key(i) = key(i + half + 1);
                    other->value(i) = value(i + half + 1);
                }

                KM_ASSERT(!other->containsInNode(middle.key));

                for (size_t i = 0; i < half + 1; i++) {
                    Leaf *leaf = child(i + half + 1);
                    other->takeChild(i, leaf);
                }

                mCount = half;
                *midpoint = middle;
            }

            void splitIntoUpperHalf(TreeNodeInternal *other, const ChildEntry& entry, Entry *midpoint) noexcept {
                /// Divide this node into another, assuming that the entry is greater than the midpoint
                /// | 1 | 2 | 3 | 4 | 5 |
                ///            ^^^^^^^^^^ entry is within this area
                /// Extract the middle node into a local
                /// Retain first M/2 entries and (M/2)+1 leaves and transfer the remaining to the other node.
                /// Then insert the new entry and leaf into the other node

                transferInto(other, midpoint);
                other->insertChild(entry.entry, entry.child);
            }

            void splitIntoLowerHalf(TreeNodeInternal *other, const ChildEntry& entry, Entry *midpoint) noexcept {
                transferInto(other, midpoint);
                this->insertChild(entry.entry, entry.child);
            }

            /// @brief Split the internal node into two nodes to accommodate a new entry and leaf.
            void splitInternalNode(TreeNodeInternal *other, const ChildEntry& entry, Entry *midpoint) noexcept {
                // guardrails
                KM_ASSERT(other->count() == 0);
                KM_ASSERT(this->count() == this->capacity());
                for (size_t i = 0; i < Super::count(); i++) {
                    if (key(i) == entry.entry.key) {
                        printf("Key %d already exists in internal node %p %d\n", (int)entry.entry.key, this, this->isLeaf());
                        this->dump();
                        KM_PANIC("Key already exists in internal node");
                    }
                }
                KM_ASSERT(!this->isLeaf());
                KM_ASSERT(!other->isLeaf());

                for (size_t i = 0; i < count(); i++) {
                    if (child(i) == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i, this);
                        this->dump();
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                }

                size_t half = capacity() / 2;
                if (entry.entry.key > key(half)) {
                    splitIntoUpperHalf(other, entry, midpoint);
                } else {
                    splitIntoLowerHalf(other, entry, midpoint);
                }

                KM_ASSERT(this->count() >= (this->capacity() / 2) - 1);
                KM_ASSERT(other->count() >= (other->capacity() / 2) - 1);
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
                for (size_t i = 0; i < count() + 1; i++) {
                    deleteNode<Key, Value>(child(i));
                }
            }

            void validate() const noexcept {
                auto keySet = Super::keys();
                bool sorted = std::is_sorted(keySet.begin(), keySet.end());
                if (!sorted) {
                    printf("Internal node %p is not sorted\n", this);
                    for (size_t i = 0; i < count(); i++) {
                        printf("Key %zu: %d\n", i, (int)key(i));
                    }
                    KM_PANIC("Internal node keys are not sorted");
                }
            }

            void dump() const {
                printf("Leaf node %p:\n", this);
                printf("  IsLeaf: %d\n", Super::isLeaf());
                printf("  Count: %zu\n", count());
                printf("  Keys: [");
                for (size_t i = 0; i < count(); i++) {
                    printf("%d, ", (int)key(i));
                }
                printf("]\n");
                printf("  Children: [");
                for (size_t i = 0; i < count() + 1; i++) {
                    printf("%p, ", child(i));
                }
                printf("]\n");
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

        template<typename Key, typename Value>
        void dumpNode(const TreeNodeLeaf<Key, Value> *node) noexcept {
            if (node->isLeaf()) {
                static_cast<const TreeNodeLeaf<Key, Value>*>(node)->dump();
            } else {
                static_cast<const TreeNodeInternal<Key, Value>*>(node)->dump();
            }
        }

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

        template<typename Key, typename Value>
        struct BTreeMapCommon {
            using Leaf = TreeNodeLeaf<Key, Value>;
            using Internal = TreeNodeInternal<Key, Value>;
            using Entry = Entry<Key, Value>;
            using ChildEntry = typename Internal::ChildEntry;

            static void splitNode(Leaf *node, Leaf *newNode, const Entry& entry, Entry *midpoint) noexcept {
                KM_ASSERT(node->isLeaf() == newNode->isLeaf());

                if (node->isLeaf()) {
                    node->splitInto(newNode, entry, midpoint);
                } else {
                    Internal *internalNode = static_cast<Internal*>(node);
                    Internal *otherInternal = static_cast<Internal*>(newNode);
                    internalNode->splitInto(otherInternal, entry, midpoint);
                }
            }

            static void deleteNode(TreeNodeHeader *node) noexcept {
                if (node->isLeaf()) {
                    Leaf *leaf = static_cast<Leaf*>(node);
                    std::destroy_at(leaf);
                } else {
                    Internal *internal = static_cast<Internal*>(node);
                    std::destroy_at(internal);
                }

                free(static_cast<void*>(node));
            }
        };
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

        struct NodeIndex {
            LeafNode *node;
            size_t index;
        };

        LeafNode *mCurrentNode;
        size_t mCurrentIndex;

        void increment() noexcept {
            if (mCurrentNode->isLeaf()) {
                while (mCurrentIndex == mCurrentNode->count() && !mCurrentNode->isRootNode()) {
                    InternalNode *parent = static_cast<InternalNode*>(static_cast<InternalNode*>(mCurrentNode)->getParent());
                    size_t newIndex = parent->indexOfChild(mCurrentNode);
                    // printf("%p:%zu <- %p:%zu\n", parent, newIndex, mCurrentNode, mCurrentIndex);
                    mCurrentIndex = newIndex;
                    mCurrentNode = parent;
                }

                if (mCurrentNode->isRootNode() && mCurrentIndex + 1 > mCurrentNode->count()) {
                    mCurrentNode = nullptr; // Reached the end of the tree
                    mCurrentIndex = 0;
                    return;
                }
            } else {
                InternalNode *childNode = static_cast<InternalNode*>(static_cast<InternalNode*>(mCurrentNode)->child(mCurrentIndex + 1));
                // printf("%p:%zu -> %p:%zu\n", mCurrentNode, mCurrentIndex, childNode, 0);
                mCurrentNode = childNode;
                while (!mCurrentNode->isLeaf()) {
                    InternalNode *newNode = static_cast<InternalNode*>(static_cast<InternalNode*>(mCurrentNode)->child(0));
                    // printf("%p:%zu -> %p:%zu\n", mCurrentNode, mCurrentIndex, newNode, 0);
                    mCurrentNode = newNode;
                }
                mCurrentIndex = 0;
            }
        }

    public:
        BTreeMapIterator(LeafNode *node, size_t index) noexcept
            : mCurrentNode(node)
            , mCurrentIndex(index)
        { }

        std::pair<const Key&, Value&> operator*() noexcept {
            auto& key = mCurrentNode->key(mCurrentIndex);
            auto& value = mCurrentNode->value(mCurrentIndex);
            // printf("%p:%zu = {%d, %d}\n", mCurrentNode, mCurrentIndex, (int)key, (int)value);
            return {key, value};
        }

        std::pair<const Key&, const Value&> operator*() const noexcept {
            auto& key = mCurrentNode->key(mCurrentIndex);
            auto& value = mCurrentNode->value(mCurrentIndex);
            // printf("%p:%zu = {%d, %d}\n", mCurrentNode, mCurrentIndex, (int)key, (int)value);
            return {key, value};
        }

        bool operator==(const BTreeMapIterator& other) const noexcept {
            return mCurrentNode == other.mCurrentNode && mCurrentIndex == other.mCurrentIndex;
        }

        bool operator!=(const BTreeMapIterator& other) const noexcept {
            return mCurrentNode != other.mCurrentNode || mCurrentIndex != other.mCurrentIndex;
        }

        BTreeMapIterator& operator++() noexcept {
            if (mCurrentNode->isLeaf() && ++mCurrentIndex < mCurrentNode->count()) {
                return *this;
            }

            increment();

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
        using Common = detail::BTreeMapCommon<Key, Value>;
        using Entry = detail::Entry<Key, Value>;
        using ChildEntry = detail::TreeNodeInternal<Key, Value>::ChildEntry;
        using RootNode = detail::TreeNodeRoot<Key, Value>;
        using InternalNode = detail::TreeNodeInternal<Key, Value>;
        using LeafNode = detail::TreeNodeLeaf<Key, Value>;

        LeafNode *mRootNode;

        void splitRootNode(LeafNode *leaf, const Entry& entry) noexcept {
            Entry midpoint;
            bool isLeaf = leaf->isLeaf();
            InternalNode *newRoot = detail::newNode<InternalNode>(nullptr);
            LeafNode *newLeaf = isLeaf ? detail::newNode<LeafNode>(newRoot) : detail::newNode<InternalNode>(newRoot);
            Common::splitNode(leaf, newLeaf, entry, &midpoint);

            newRoot->initAsRoot(leaf, newLeaf, midpoint);
            printf("New root node %p created with entry %d\n", newRoot, (int)midpoint.key);
            mRootNode = newRoot;
        }

        void splitLeafNode(LeafNode *leaf, const Entry& entry) noexcept {
            Entry midpoint;
            bool isLeaf = leaf->isLeaf();
            InternalNode *parent = static_cast<InternalNode*>(leaf->getParent());
            LeafNode *newLeaf = isLeaf ? detail::newNode<LeafNode>(parent) : detail::newNode<InternalNode>(parent);
            Common::splitNode(leaf, newLeaf, entry, &midpoint);

            insertNewLeaf(parent, newLeaf, midpoint);
        }

        void splitNode(LeafNode *leaf, const Entry& entry) noexcept {
            if (leaf->find(entry.key) != nullptr) {
                printf("Key %d already exists in node %p\n", (int)entry.key, leaf);
                leaf->dump();
                KM_PANIC("Key already exists in node");
            }

            // Splitting the root node is a special case.
            if (leaf == mRootNode) {
                splitRootNode(leaf, entry);
            } else {
                splitLeafNode(leaf, entry);
            }
        }

        void splitInternalNode(InternalNode *parent, const Entry& entry, LeafNode *leaf) noexcept {
            InternalNode *newNode = detail::newNode<InternalNode>(parent->getParent());

            if (parent == mRootNode) {
                Entry midpoint;
                parent->splitInternalNode(newNode, ChildEntry{leaf, entry}, &midpoint);
                mRootNode = detail::newNode<RootNode>(parent, newNode, midpoint);
            } else {
                Entry midpoint;
                parent->splitInternalNode(newNode, ChildEntry{leaf, entry}, &midpoint);
                insertNewLeaf(static_cast<InternalNode*>(parent->getParent()), newNode, midpoint);
            }
        }

        /// @brief Insert a new node into an internal node.
        ///
        /// @param parent The parent internal node where the new leaf will be inserted.
        /// @param newLeaf The new leaf node to be inserted.
        void insertNewLeaf(InternalNode *parent, LeafNode *newLeaf, const Entry& midpoint) noexcept {
            detail::InsertResult result = parent->insertChild(midpoint, newLeaf);
            if (result == detail::InsertResult::eFull) {
                splitInternalNode(parent, midpoint, newLeaf);
            }
        }

        void insertInto(LeafNode *node, const Entry& entry) noexcept {
            if (node->isLeaf()) {
                detail::InsertResult result = node->insert(entry);
                if (result == detail::InsertResult::eFull) {
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
                    if (child == nullptr) {
                        internal->dump();
                        printf("Child at index %zu is null in node %p\n", index, internal);
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                    return nodeContains(child, key);
                }
            }
        }

        /// @brief Merge two leaf nodes into one.
        ///
        /// @param leaf The leaf to merge into.
        /// @param other The other leaf to merge from.
        /// @param middle The entry that is between the two nodes.
        void mergeLeafNode(LeafNode *leaf, LeafNode *other, const Entry& middle) noexcept {
            KM_ASSERT(leaf->isLeaf() == other->isLeaf());
            KM_ASSERT(leaf->getParent() == other->getParent());
            KM_ASSERT(leaf->capacity() < (leaf->count() + other->count() + 1));

            leaf->insert(middle);
            size_t start = leaf->count();
            leaf->mCount += other->count();
            for (size_t i = 0; i < other->count(); i++) {
                leaf->emplace(i + start, {other->key(i), other->value(i)});
            }
            other->mCount = 0; // Clear the other node
            other->setParent(nullptr); // Detach the other node from the tree
            Common::deleteNode(other); // Free the memory of the other node
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
                Common::deleteNode(mRootNode);
                mRootNode = nullptr;
            }
        }

        void insert(const Key& key, const Value& value) noexcept {
            if (mRootNode == nullptr) {
                mRootNode = detail::newNode<LeafNode>(nullptr);
            }

            insertInto(mRootNode, Entry{key, value});
        }

        void remove(const Key& key) noexcept {
            erase(find(key));
        }

        void erase(Iterator it) noexcept {
            if (it == end()) {
                return; // Nothing to erase
            }

            // TODO: this

            LeafNode *node = it.mCurrentNode;
            if (node->isRootNode()) {
                if (node->isLeaf()) {
                    node->remove(it.mCurrentIndex);
                } else {
                    node->removeAndPromoteChild(it.mCurrentIndex);
                }
            } else {
                KM_PANIC("Unimplemented");
            }
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
                    size_t index = internal->findChildIndex(key);
                    if (index != 0 && internal->key(index - 1) == key) {
                        return Iterator(current, index - 1); // Found the key in the internal node
                    }
                    current = internal->child(index);
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

        void dump() const noexcept {
            if (mRootNode == nullptr) {
                printf("BTreeMap is empty\n");
                return;
            }

            auto dumpNode = [&](this auto&& self, const LeafNode *node, int currentDepth) {
                for (int i = 0; i < currentDepth; i++) {
                    printf("  ");
                }
                if (node == nullptr) {
                    printf("Node is null at depth %d\n", currentDepth);
                    return;
                }
                if (node->isLeaf()) {
                    printf("Leaf node %p at depth %d with %zu keys: [", node, currentDepth, node->count());
                    for (size_t i = 0; i < node->count(); i++) {
                        printf("%d, ", (int)node->key(i));
                    }
                    printf("]\n");
                } else {
                    const InternalNode *internal = static_cast<const InternalNode*>(node);
                    printf("Internal node %p at depth %d with %zu keys: [", internal, currentDepth, internal->count());
                    for (size_t i = 0; i < internal->count(); i++) {
                        printf("%d, ", (int)internal->key(i));
                    }
                    printf("]\n");
                    for (size_t i = 0; i < internal->count() + 1; i++) {
                        self(internal->getLeaf(i), currentDepth + 1);
                    }
                }
            };

            printf("BTreeMap root node %p:\n", mRootNode);
            dumpNode(mRootNode, 0);
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
                    size_t lowerBound = (node->capacity() / 2) - 1;
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
                    if (prev == nullptr || next == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i, internalNode);
                        this->dump();
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                    for (size_t j = 0; j < prev->count(); j++) {
                        if (prev->key(j) >= internalNode->key(i)) {
                            printf("Key %d in previous node %p at depth %zu is not less than key %d in internal node %p\n",
                                   (int)prev->key(j), prev, currentDepth, (int)internalNode->key(i), internalNode);

                            dump();
                        }
                        KM_ASSERT(prev->key(j) < internalNode->key(i));
                    }

                    for (size_t j = 0; j < next->count(); j++) {
                        if (internalNode->key(i) >= next->key(j)) {
                            printf("Key %d in internal node %p at depth %zu is not less than key %d in next node %p\n",
                                   (int)internalNode->key(i), internalNode, currentDepth, (int)next->key(j), next);
                            dump();
                        }
                        KM_ASSERT(internalNode->key(i) < next->key(j));
                    }
                }

                for (size_t i = 0; i < internalNode->count() + 1; i++) {
                    LeafNode *child = internalNode->getLeaf(i);
                    if (child == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i, internalNode);
                        this->dump();
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                    KM_ASSERT(child->getParent() == internalNode);
                    validateNode(child, currentDepth + 1);
                }
                validateNode(internalNode->getLeaf(internalNode->count()), currentDepth + 1); // Last child
            };

            validateNode(mRootNode, 1);
        }
    };
}
