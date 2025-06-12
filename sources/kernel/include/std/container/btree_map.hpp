#pragma once

#include "panic.hpp"
#include "util/memory.hpp"

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

            constexpr size_t capacity() const noexcept {
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

            InsertResult insert(LeafNode *child) noexcept {
                size_t n = count();
                Key k = child->min();

                for (size_t i = 0; i < n; i++) {
                    if (key(i) > k) {
                        if (n >= capacity()) {
                            return InsertResult::eFull;
                        }

                        // Shift the keys and values to make space for the new key.
                        setCount(n + 1);
                        for (size_t j = n; j > i; j--) {
                            emplace(j, key(j - 1));
                        }
                        for (size_t j = n; j > i; j--) {
                            child(j + 1) = child(j);
                        }
                        emplace(i, child);
                        return InsertResult::eSuccess;
                    }
                }

                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                // If we reach here, the key is greater than all existing keys.
                setCount(n + 1);
                emplace(n, child);
                return InsertResult::eSuccess;
            }

            void emplace(size_t index, LeafNode *child) noexcept {
                KM_ASSERT(index < count() + 1);
                mKeys[index] = child->min();
                takeNode(index, child);
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
            void *ptr = aligned_alloc(alignof(T), sizeof(T));
            return new (ptr) T(std::forward<Args>(args)...);
        }
    };
}
