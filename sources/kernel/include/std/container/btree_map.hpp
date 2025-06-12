#pragma once

#include "panic.hpp"
#include "util/memory.hpp"

#include "common/util/util.hpp"

#include <stdlib.h>

namespace sm::detail {
    enum class InsertResult {
        eSuccess,
        eFull,
    };

    template<typename Key, typename Value>
    struct BTreeMapCommon {
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
                mKeys[index] = entry.key;
                mValues[index] = entry.value;
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
        };

        static constexpr size_t kInternalOrder = std::max(3zu, (kTargetSize - sizeof(BaseNode)) / (sizeof(Key) + sizeof(LeafNode*)));

        class InternalNode : public BaseNode {
            using Super = BaseNode;

            Key mKeys[kInternalOrder];
            LeafNode *mChildren[kInternalOrder + 1];

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

            std::span<LeafNode*> children() noexcept {
                return std::span<LeafNode*>(mChildren, count() + 1);
            }

            std::span<const LeafNode*> children() const noexcept {
                return std::span<const LeafNode*>(mChildren, count() + 1);
            }

            LeafNode *&child(size_t index) noexcept {
                KM_ASSERT(index < count() + 1);
                return mChildren[index];
            }

            LeafNode * const& child(size_t index) const noexcept {
                KM_ASSERT(index < count() + 1);
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

            InternalNode(InternalNode *parent) noexcept
                : BaseNode(false, parent)
            { }

            ~InternalNode() noexcept {
                for (size_t i = 0; i < count() + 1; i++) {
                    destroyNode(mChildren[i]);
                }
            }

            void takeNode(size_t index, BaseNode *node) noexcept {
                KM_ASSERT(index < count() + 1);
                node->setParent(this);
                mChildren[index] = static_cast<LeafNode*>(node);
            }

            void transferTo(InternalNode *other, size_t size) noexcept {
                KM_ASSERT(other->isEmpty());

                size_t start = count() - size;

                other->setCount(size);

                for (size_t i = 0; i < size; i++) {
                    other->key(i) = key(i + start + 1);
                }

                for (size_t i = 0; i < size + 1; i++) {
                    other->takeNode(i, child(i + start));
                }

                setCount(count() - size);
            }

            void splitInternalInto(InternalNode *other, LeafNode *leaf) noexcept {

            }

            InsertResult insert(LeafNode *leaf) noexcept {
                size_t n = count();
                Key k = leaf->min();

                for (size_t i = 0; i < n; i++) {
                    if (key(i) > k) {
                        if (n >= capacity()) {
                            return InsertResult::eFull;
                        }

                        // Shift the keys and values to make space for the new key.
                        setCount(n + 1);
                        for (size_t j = n; j > i; j--) {
                            key(j) = key(j - 1);
                        }
                        for (size_t j = n; j > i; j--) {
                            child(j + 1) = child(j);
                        }
                        key(i) = k;
                        takeNode(i + 1, leaf);
                        return InsertResult::eSuccess;
                    }
                }

                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                // If we reach here, the key is greater than all existing keys.
                setCount(n + 1);
                key(n) = k;
                takeNode(n + 1, leaf);
                return InsertResult::eSuccess;
            }

            void emplace(size_t index, LeafNode *child) noexcept {
                KM_ASSERT(index < count() + 1);
                mKeys[index] = child->min();
                takeNode(index, child);
            }

            void initAsRoot(LeafNode *lhs, LeafNode *rhs) noexcept {
                KM_ASSERT(isRootNode());
                setCount(1);
                mKeys[0] = rhs->min();
                takeNode(0, lhs);
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
    class BTreeMap {
        using Common = sm::detail::BTreeMapCommon<Key, Value>;
        using Node = Common::BaseNode;
        using Leaf = Common::LeafNode;
        using Internal = Common::InternalNode;
        using Entry = Common::NodeEntry;

        template<typename T, typename... Args>
        static T *newNode(Args&&... args) noexcept {
            return Common::template newNode<T>(std::forward<Args>(args)...);
        }

        Node *mRoot;

        void splitInternalNode(Internal *node, Leaf *leaf) noexcept {

        }

        void insertNewLeaf(Internal *parent, Leaf *leaf) noexcept {
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
                splitNode(node, entry);
            }
        }

        void insertIntoInternal(Internal *node, const Entry& entry) noexcept {

        }

        void insert(Node *node, const Entry& entry) noexcept {
            if (node->isLeaf()) {
                insertIntoLeaf(static_cast<Leaf*>(node), entry);
            } else {
                insertIntoInternal(static_cast<Internal*>(node), entry);
            }
        }
    public:
        BTreeMap() noexcept
            : mRoot(nullptr)
        { }

        UTIL_NOCOPY(BTreeMap);
        UTIL_NOMOVE(BTreeMap);

        ~BTreeMap() noexcept {
            if (mRoot) {
                Common::destroyNode(mRoot);
                mRoot = nullptr;
            }
        }

        void insert(const Key& key, const Value& value) noexcept {
            if (mRoot == nullptr) {
                mRoot = newNode<Leaf>(nullptr);
            }

            insert(mRoot, Entry{key, value});
        }

        bool contains(const Key& key) const noexcept {
            if (mRoot == nullptr) {
                return false;
            }

            return false;
        }
    };
}
