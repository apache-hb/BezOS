#pragma once

#include "panic.hpp"
#include "util/memory.hpp"

#include "common/util/util.hpp"
#include "std/container/detail/btree.hpp"

#include <stdlib.h>

namespace sm::detail {
    template<typename Key, typename Value>
    struct BPlusTreeMapCommon {
        static constexpr size_t kTargetSize = sm::kilobytes(4).bytes();

        class BaseNode;
        class LeafNode;
        class InternalNode;

        struct NodeEntry {
            Key key;
            Value value;
        };

        class BaseNode {
            bool mIsLeaf;
            uint16_t mCount;
            InternalNode *mParent;
        public:
            BaseNode(bool isLeaf, InternalNode *parent) noexcept
                : mIsLeaf(isLeaf)
                , mCount(0)
                , mParent(parent)
            { }

            virtual ~BaseNode() noexcept = default;

            virtual bool contains(const Key& key) const noexcept = 0;
            virtual void validate() const noexcept = 0;
            virtual Key readMinKey() const noexcept = 0;
            virtual Key readMaxKey() const noexcept = 0;
            virtual void dump(int depth) const noexcept = 0;

            size_t count() const noexcept {
                return mCount;
            }

            void setCount(size_t count) noexcept {
                mCount = static_cast<uint16_t>(count);
            }

            bool isLeaf() const noexcept {
                return mIsLeaf;
            }

            InternalNode *getParent() const noexcept {
                return mParent;
            }

            void setParent(InternalNode *parent) noexcept {
                mParent = parent;
            }

            bool isRootNode() const noexcept {
                return getParent() == nullptr;
            }
        };

        static constexpr size_t kLeafOrder = std::max(3zu, (kTargetSize - sizeof(BaseNode)) / (sizeof(Key) + sizeof(Value)));

        class LeafNode : public BaseNode {
            using Super = BaseNode;

            Key mKeys[kLeafOrder];
            Value mValues[kLeafOrder];

            void emplace(size_t index, const NodeEntry& entry) noexcept {
                KM_ASSERT(index < count() + 1);
                key(index) = entry.key;
                value(index) = entry.value;
            }

            using Super::setCount;
        public:
            using Super::count;
            using Super::isRootNode;

            std::span<Key> keys() noexcept {
                return std::span<Key>(mKeys, count());
            }

            std::span<const Key> keys() const noexcept {
                return std::span<const Key>(mKeys, count());
            }

            std::span<Value> values() noexcept {
                return std::span<Value>(mValues, count());
            }

            std::span<const Value> values() const noexcept {
                return std::span<const Value>(mValues, count());
            }

            Key& key(size_t index) noexcept {
                KM_ASSERT(index < count());
                return mKeys[index];
            }

            const Key& key(size_t index) const noexcept {
                KM_ASSERT(index < count());
                return mKeys[index];
            }

            Value& value(size_t index) noexcept {
                KM_ASSERT(index < count());
                return mValues[index];
            }

            const Value& value(size_t index) const noexcept {
                KM_ASSERT(index < count());
                return mValues[index];
            }

            LeafNode(InternalNode *parent) noexcept
                : BaseNode(true, parent)
            { }

            constexpr size_t capacity() const noexcept {
                return kLeafOrder;
            }

            static constexpr size_t maxCapacity() noexcept {
                return kLeafOrder;
            }

            bool isFull() const noexcept {
                return count() == capacity();
            }

            bool isEmpty() const noexcept {
                return count() == 0;
            }

            /// @brief Transfer the the upper contents of this leaf node to another leaf node.
            ///
            /// @param other The leaf node to transfer contents to.
            /// @param size The number of entries to transfer.
            void transferTo(LeafNode *other, size_t size) noexcept {
                KM_ASSERT(other->isEmpty());

                size_t start = count() - size;
                other->setCount(size);
                for (size_t i = 0; i < size; i++) {
                    other->emplace(i, get(i + start));
                }
                setCount(count() - size);
            }

            void splitLeafInto(LeafNode *other, const NodeEntry& entry) noexcept {
                KM_ASSERT(isFull());

                bool isOdd = (count() & 1);
                size_t half = capacity() / 2;
                size_t otherSize = half + (isOdd ? 1 : 0);
                NodeEntry middle = get(half);

                transferTo(other, otherSize);

                if (entry.key < middle.key) {
                    this->insert(entry);
                } else {
                    other->insert(entry);
                }
            }

            InsertResult insert(const NodeEntry& entry) noexcept {
                size_t n = count();

                for (size_t i = 0; i < n; i++) {
                    if (mKeys[i] == entry.key) {
                        mValues[i] = entry.value;
                        return InsertResult::eSuccess;
                    } else if (mKeys[i] > entry.key) {
                        if (n >= capacity()) {
                            return InsertResult::eFull;
                        }

                        // Shift the keys and values to make space for the new key.
                        setCount(n + 1);
                        for (size_t j = n; j > i; j--) {
                            emplace(j, get(j - 1));
                        }
                        emplace(i, entry);
                        return InsertResult::eSuccess;
                    }
                }

                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                // If we reach here, the key is greater than all existing keys.
                setCount(n + 1);
                emplace(n, entry);
                return InsertResult::eSuccess;
            }

            NodeEntry get(size_t index) const noexcept {
                KM_ASSERT(index < count());
                return {mKeys[index], mValues[index]};
            }

            NodeEntry popHead() noexcept {
                KM_ASSERT(!isEmpty());
                NodeEntry entry = head();
                for (size_t i = 1; i < count(); i++) {
                    emplace(i - 1, get(i));
                }
                setCount(count() - 1);
                return entry;
            }

            NodeEntry popTail() noexcept {
                KM_ASSERT(!isEmpty());
                NodeEntry entry = tail();
                setCount(count() - 1);
                return entry;
            }

            NodeEntry head() const noexcept {
                KM_ASSERT(!isEmpty());
                return get(0);
            }

            NodeEntry tail() const noexcept {
                KM_ASSERT(!isEmpty());
                return get(count() - 1);
            }

            Key min() const noexcept {
                KM_ASSERT(!isEmpty());
                return key(0);
            }

            Key max() const noexcept {
                KM_ASSERT(!isEmpty());
                return key(count() - 1);
            }

            bool contains(const Key& key) const noexcept override {
                for (size_t i = 0; i < count(); i++) {
                    if (mKeys[i] == key) {
                        return true;
                    }
                }
                return false;
            }

            Key readMinKey() const noexcept override {
                return min();
            }

            Key readMaxKey() const noexcept override {
                return max();
            }

            void dump(int depth) const noexcept override {
                for (int i = 0; i < depth; i++) {
                    printf("  ");
                }
                printf("%p: %zu [ ", (void*)this, count());
                for (size_t i = 0; i < count(); i++) {
                    printf("%d ", (int)mKeys[i]);
                }
                printf("]\n");
            }

            void validate() const noexcept override {
                KM_ASSERT(count() <= capacity());
                auto ketSet = keys();
                bool isSorted = std::is_sorted(ketSet.begin(), ketSet.end());

                if (!isSorted) {
                    printf("Leaf node %p is not sorted\n", (void*)this);
                    for (size_t i = 0; i < count(); i++) {
                        printf("Key %zu: %d\n", i, (int)mKeys[i]);
                    }
                    KM_PANIC("Leaf node keys are not sorted");
                }
            }
        };

        static constexpr size_t kInternalOrder = std::max(3zu, (kTargetSize - sizeof(BaseNode)) / (sizeof(Key) + sizeof(LeafNode*)));

        class InternalNode : public BaseNode {
            using Super = BaseNode;

            Key mKeys[kInternalOrder];
            BaseNode *mChildren[kInternalOrder];

            using Super::setCount;
        public:
            using Super::count;
            using Super::isRootNode;

            std::span<Key> keys() noexcept {
                return std::span<Key>(mKeys, count());
            }

            std::span<const Key> keys() const noexcept {
                return std::span<const Key>(mKeys, count());
            }

            Key& key(size_t index) noexcept {
                KM_ASSERT(index < count());
                return mKeys[index];
            }

            const Key& key(size_t index) const noexcept {
                KM_ASSERT(index < count());
                return mKeys[index];
            }

            std::span<BaseNode*> children() noexcept {
                return std::span<BaseNode*>(mChildren, count());
            }

            std::span<const BaseNode*> children() const noexcept {
                return std::span<const BaseNode*>(mChildren, count());
            }

            BaseNode *&child(size_t index) noexcept {
                KM_ASSERT(index < count());
                return mChildren[index];
            }

            BaseNode * const& child(size_t index) const noexcept {
                KM_ASSERT(index < count());
                return mChildren[index];
            }

            constexpr size_t capacity() const noexcept {
                return kInternalOrder;
            }

            static constexpr size_t maxCapacity() noexcept {
                return kInternalOrder;
            }

            bool isFull() const noexcept {
                return count() == capacity();
            }

            bool isEmpty() const noexcept {
                return count() == 0;
            }

            size_t upperBound(const Key& key) const noexcept {
                for (size_t i = 0; i < count() - 1; i++) {
                    if (key < mKeys[i]) {
                        return i;
                    }
                }
                return count() - 1; // Return the last index if key is greater than all keys
            }

            size_t childIndex(const BaseNode *node) const noexcept {
                for (size_t i = 0; i < count(); i++) {
                    if (mChildren[i] == node) {
                        return i;
                    }
                }

                return SIZE_MAX; // Not found
            }

            InternalNode(InternalNode *parent) noexcept
                : BaseNode(false, parent)
            { }

            ~InternalNode() noexcept {
                for (size_t i = 0; i < count(); i++) {
                    destroyNode(mChildren[i]);
                }
            }

            void takeNode(size_t index, BaseNode *node) noexcept {
                KM_ASSERT(index < count());
                node->setParent(this);
                mChildren[index] = node;
            }

            void transferTo(InternalNode *other, size_t size) noexcept {
                KM_ASSERT(other->isEmpty());

                size_t start = count() - size;

                other->setCount(size);

                for (size_t i = 0; i < size; i++) {
                    other->key(i) = key(i + start);
                }

                for (size_t i = 0; i < size; i++) {
                    other->takeNode(i, child(i + start));
                }

                setCount(count() - size);
            }

            void splitInternalInto(InternalNode *other, BaseNode *leaf) noexcept {
                KM_ASSERT(isFull());
                size_t half = capacity() / 2;

                transferTo(other, half);

                if (minKey(leaf) < minKey(other)) {
                    KM_ASSERT(this->insert(leaf) == InsertResult::eSuccess);
                } else {
                    KM_ASSERT(other->insert(leaf) == InsertResult::eSuccess);
                }
            }

            InsertResult insert(BaseNode *leaf) noexcept {
                size_t size = count();
                if (size == capacity()) {
                    return InsertResult::eFull;
                }

                Key k = minKey(leaf);

                for (size_t i = 0; i < size; i++) {
                    if (k < key(i)) {
                        // Shift the keys and values to make space for the new key.
                        setCount(size + 1);
                        for (size_t j = size; j > i; j--) {
                            key(j) = key(j - 1);
                            child(j) = child(j - 1);
                        }
                        key(i) = k;
                        takeNode(i, leaf);
                        return InsertResult::eSuccess;
                    }
                }

                // If we reach here, the key is greater than all existing keys.
                setCount(size + 1);
                key(size) = k;
                takeNode(size, leaf);
                return InsertResult::eSuccess;
            }

            void emplace(size_t index, BaseNode *child) noexcept {
                KM_ASSERT(index < count() + 1);
                mKeys[index] = minKey(child);
                takeNode(index, child);
            }

            void initAsRoot(BaseNode *lhs, BaseNode *rhs) noexcept {
                KM_ASSERT(isRootNode());
                setCount(2);

                mKeys[0] = minKey(lhs);
                takeNode(0, lhs);

                mKeys[1] = minKey(rhs);
                takeNode(1, rhs);
            }

            Key min() const noexcept {
                KM_ASSERT(!isEmpty());
                return key(0);
            }

            Key max() const noexcept {
                KM_ASSERT(!isEmpty());
                return key(count() - 1);
            }

            bool contains(const Key& key) const noexcept override {
                size_t index = upperBound(key);
                BaseNode *node = child(index);
                return node->contains(key);
            }

            Key readMinKey() const noexcept override {
                return child(0)->readMinKey();
            }

            Key readMaxKey() const noexcept override {
                return child(count())->readMaxKey();
            }

            void dump(int depth) const noexcept override {
                for (int i = 0; i < depth; i++) {
                    printf("  ");
                }
                printf("%p: %zu [ ", (void*)this, count());
                for (size_t i = 0; i < count(); i++) {
                    printf("%d ", (int)mKeys[i]);
                }
                printf("]\n");
                for (size_t i = 0; i < count(); i++) {
                    child(i)->dump(depth + 1);
                }
            }

            void validate() const noexcept override {
                KM_ASSERT(count() <= capacity());
                auto keySet = keys();
                bool isSorted = std::is_sorted(keySet.begin(), keySet.end());

                if (!isSorted) {
                    printf("Internal node %p is not sorted\n", (void*)this);
                    for (size_t i = 0; i < count(); i++) {
                        printf("Key %zu: %d\n", i, (int)mKeys[i]);
                    }
                    KM_PANIC("Internal node keys are not sorted");
                }

                for (size_t i = 0; i < count(); i++) {
                    auto min = minKey(child(i));
                    if (min != key(i)) {
                        printf("%d != %d\n", (int)min, (int)key(i));
                        dump(0);
                    }
                    KM_ASSERT(min == key(i));
                }

                if (count() != 1) {
                    for (size_t i = 0; i < count() - 1; i++) {
                        auto lhsMax = child(i)->readMaxKey(); // maxKey(child(i));
                        auto rhsMin = child(i + 1)->readMinKey(); // minKey(child(i + 1));
                        if (lhsMax >= rhsMin) {
                            printf("Internal node %p has overlapping keys: %d >= %d\n", (void*)this, (int)lhsMax, (int)rhsMin);
                        }
                        KM_ASSERT(lhsMax < rhsMin);
                    }
                }

                for (size_t i = 0; i < count(); i++) {
                    BaseNode *childNode = child(i);
                    KM_ASSERT(childNode != nullptr);
                    KM_ASSERT(childNode->getParent() == this);
                    childNode->validate();
                }
            }
        };

        static void destroyNode(BaseNode *node) noexcept {
            if (node->isLeaf()) {
                LeafNode *leaf = static_cast<LeafNode*>(node);
                std::destroy_at(leaf);
            } else {
                InternalNode *internal = static_cast<InternalNode*>(node);
                std::destroy_at(internal);
            }

            free(static_cast<void*>(node));
        }

        static bool nodeContains(const BaseNode *node, const Key& key) noexcept {
            if (node->isLeaf()) {
                const LeafNode *leaf = static_cast<const LeafNode*>(node);
                for (size_t i = 0; i < leaf->count(); i++) {
                    if (leaf->key(i) == key) {
                        return true;
                    }
                }
                return false;
            } else {
                const InternalNode *internal = static_cast<const InternalNode*>(node);
                for (size_t i = 0; i < internal->count(); i++) {
                    if (internal->key(i) < key) {
                        return nodeContains(internal->child(i), key);
                    }
                }

                // If we reach here, the key is greater than all existing keys.
                return nodeContains(internal->child(internal->count()), key);
            }
        }

        static Key minKey(const BaseNode *node) noexcept {
            if (node->isLeaf()) {
                const LeafNode *leaf = static_cast<const LeafNode*>(node);
                return leaf->min();
            } else {
                const InternalNode *internal = static_cast<const InternalNode*>(node);
                return internal->min();
            }
        }

        static Key maxKey(const BaseNode *node) noexcept {
            if (node->isLeaf()) {
                const LeafNode *leaf = static_cast<const LeafNode*>(node);
                return leaf->max();
            } else {
                const InternalNode *internal = static_cast<const InternalNode*>(node);
                return internal->max();
            }
        }

        template<typename T, typename... Args>
        static T* newNode(Args&&... args) noexcept {
            if (void *ptr = aligned_alloc(alignof(T), sizeof(T))) {
                return new (ptr) T(std::forward<Args>(args)...);
            }

            return nullptr;
        }
    };
}

namespace sm {
    template<typename Key, typename Value>
    class BPlusTreeMap {
        using Common = sm::detail::BPlusTreeMapCommon<Key, Value>;
        using Node = Common::BaseNode;
        using Leaf = Common::LeafNode;
        using Internal = Common::InternalNode;
        using Entry = Common::NodeEntry;

        template<typename T, typename... Args>
        static T *newNode(Args&&... args) noexcept {
            return Common::template newNode<T>(std::forward<Args>(args)...);
        }

        Node *mRoot;

        void splitInternalRootNode(Internal *node, Node *leaf) noexcept {
            Internal *parent = newNode<Internal>(nullptr);
            Internal *other = newNode<Internal>(parent);
            node->splitInternalInto(other, leaf);
            parent->initAsRoot(node, other);
            mRoot = parent;
        }

        void splitInternalChildNode(Internal *node, Node *leaf) noexcept {
            Internal *parent = node->getParent();
            Internal *other = newNode<Internal>(parent);
            node->splitInternalInto(other, leaf);
            insertNewLeaf(parent, other);
        }

        void splitInternalNode(Internal *node, Node *leaf) noexcept {
            if (node == mRoot) {
                splitInternalRootNode(node, leaf);
            } else {
                splitInternalChildNode(node, leaf);
            }
        }

        void insertNewLeaf(Internal *parent, Node *leaf) noexcept {
            if (parent->isFull()) {
                splitInternalNode(parent, leaf);
            } else {
                parent->insert(leaf);
            }
        }

        void splitLeafChildNode(Leaf *node, const Entry& entry) noexcept {
            Internal *parent = node->getParent();
            Leaf *newLeaf = newNode<Leaf>(parent);
            node->splitLeafInto(newLeaf, entry);
            insertNewLeaf(parent, newLeaf);
        }

        void splitLeafRootNode(Leaf *node, const Entry& entry) noexcept {
            Internal *parent = newNode<Internal>(nullptr);
            Leaf *newLeaf = newNode<Leaf>(parent);
            node->splitLeafInto(newLeaf, entry);
            parent->initAsRoot(node, newLeaf);
            mRoot = parent;
        }

        void splitLeafNode(Leaf *node, const Entry& entry) noexcept {
            if (node == mRoot) {
                splitLeafRootNode(node, entry);
            } else {
                splitLeafChildNode(node, entry);
            }
        }

        void insertIntoLeaf(Leaf *node, const Entry& entry) noexcept {
            detail::InsertResult result = node->insert(entry);
            if (result == detail::InsertResult::eFull) {
                splitLeafNode(node, entry);
            }
        }

        void insertIntoInternal(Internal *node, const Entry& entry) noexcept {
            for (size_t i = 0; i < node->count(); i++) {
                if (entry.key < node->key(i)) {
                    Node *child = node->child(i);
                    insertIntoNode(child, entry);
                    return;
                }
            }

            Internal *parent = node->getParent();
            if (parent != nullptr) {
                size_t thisIndex = parent->childIndex(node);
                printf("Migrate %zu %zu\n", thisIndex, parent->count());
                if (thisIndex != parent->count() - 1) {
                    Node *nextChild = parent->child(thisIndex + 1);
                    printf("Inner %d %d\n", (int)entry.key, (int)Common::minKey(nextChild));
                    if (entry.key < Common::minKey(nextChild) && Common::maxKey(node) < entry.key) {
                        insertIntoNode(nextChild, entry);
                        return;
                    }
                }
            }

            // If we reach here, the key is greater than all existing keys.
            Node *child = node->child(node->count() - 1);
            insertIntoNode(child, entry);
        }

        void insertIntoNode(Node *node, const Entry& entry) noexcept {
            if (node->isLeaf()) {
                insertIntoLeaf(static_cast<Leaf*>(node), entry);
            } else {
                insertIntoInternal(static_cast<Internal*>(node), entry);
            }
        }
    public:
        BPlusTreeMap() noexcept
            : mRoot(nullptr)
        { }

        UTIL_NOCOPY(BPlusTreeMap);
        UTIL_NOMOVE(BPlusTreeMap);

        ~BPlusTreeMap() noexcept {
            if (mRoot) {
                Common::destroyNode(mRoot);
                mRoot = nullptr;
            }
        }

        void insert(const Key& key, const Value& value) noexcept {
            if (mRoot == nullptr) {
                mRoot = newNode<Leaf>(nullptr);
            }

            insertIntoNode(mRoot, Entry{key, value});
        }

        bool contains(const Key& key) const noexcept {
            if (mRoot == nullptr) {
                return false;
            }

            return mRoot->contains(key); // Common::nodeContains(mRoot, key);
        }

        void validate() const noexcept {
            if (mRoot == nullptr) {
                return; // Empty tree is valid
            }

            mRoot->validate();
        }

        void dump() const noexcept {
            if (mRoot == nullptr) {
                printf("BTreeMap is empty\n");
                return;
            }

            mRoot->dump(0);
        }
    };
}
