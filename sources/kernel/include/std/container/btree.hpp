#pragma once

#include "allocator/allocator.hpp"
#include "common/util/util.hpp"

#include "std/layout.hpp"
#include "util/memory.hpp"

#include "panic.hpp"

#include <functional>

#include <limits.h>
#include <stdlib.h>

namespace sm::detail {
    enum class InsertResult {
        eSuccess,
        eFull,
    };

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

    /// @brief Dont use this, its here because I cant bother to rewrite test cases. Always use the allocator instead.
    template<typename Node, typename... Args>
    Node *newNode(Args&&... args) noexcept {
        void *memory = aligned_alloc(alignof(Node), sizeof(Node));
        return new (memory) Node(std::forward<Args>(args)...);
    }

    static constexpr bool kUseLowerBoundForLeafInsert = true;
    static constexpr bool kUseLowerBoundForLeafFind = true;
    static constexpr bool kUseLowerBoundForInternalInsert = true;
    static constexpr bool kUseUpperBoundForInternalFind = true;
    static constexpr bool kTreeDebug = false;
    static constexpr size_t kTargetNodeSize = sm::kilobytes(1).bytes();

    template<typename Key, typename Value>
    struct BTreeMapCommon {
        struct Entry {
            Key key;
            Value value;
        };

        class Leaf;

        using KeySpan = std::span<Key>;
        using ConstKeySpan = std::span<const Key>;
        using ValueSpan = std::span<Value>;
        using ConstValueSpan = std::span<const Value>;
        using ChildSpan = std::span<Leaf*>;
        using ConstChildSpan = std::span<Leaf* const>;

        using KeyIterator = typename KeySpan::iterator;
        using ValueIterator = typename ValueSpan::iterator;
        using ChildIterator = typename ChildSpan::iterator;

        class Leaf : public TreeNodeHeader {
            using Super = TreeNodeHeader;
        public:
            static constexpr size_t kOrder = std::max(3zu, computeMaxLeafOrder(Layout::of<Key>(), Layout::of<Value>(), kTargetNodeSize));

        private:
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

            size_t start() const noexcept {
                return 0;
            }

        public:
            Entry popBack() noexcept {
                KM_ASSERT(count() > 0);
                Entry entry = {key(count() - 1), value(count() - 1)};
                setCount(count() - 1);
                return entry;
            }

            Entry popFront() noexcept {
                KM_ASSERT(count() > 0);
                Entry entry = {key(0), value(0)};
                auto keySet = keys();
                auto valueSet = values();
                std::move(keySet.begin() + 1, keySet.end(), keySet.begin());
                std::move(valueSet.begin() + 1, valueSet.end(), valueSet.begin());
                setCount(count() - 1);
                return entry;
            }

            void emplace(size_t index, const Entry& entry) noexcept {
                key(index) = entry.key;
                value(index) = entry.value;
            }

            auto& key(this auto&& self, size_t index) noexcept {
                return self.mKeys[index];
            }

            auto& value(this auto&& self, size_t index) noexcept {
                return self.mValues[index];
            }

            Leaf(TreeNodeHeader *parent) noexcept
                : Leaf(true, parent)
            { }

            Leaf(bool isLeaf, TreeNodeHeader *parent) noexcept
                : Super(isLeaf, parent)
            { }

            InsertResult insertViaLowerBound(const Entry& entry) noexcept {
                size_t index = lowerBound(entry.key);
                size_t n = count();

                if (index == count()) {
                    if (n >= capacity()) {
                        return InsertResult::eFull;
                    }
                    setCount(n + 1);
                    emplace(n, entry);
                } else if (key(index) == entry.key) {
                    value(index) = entry.value;
                } else {
                    if (n >= capacity()) {
                        return InsertResult::eFull;
                    }
                    // Shift the keys and values to make space for the new key.
                    setCount(n + 1);
                    shiftEntry(index);
                    emplace(index, entry);
                }

                return InsertResult::eSuccess;
            }

            InsertResult insertViaScan(const Entry& entry) noexcept {
                size_t n = count();

                for (size_t i = start(); i < n; i++) {
                    if (key(i) == entry.key) {
                        value(i) = entry.value;
                        return InsertResult::eSuccess;
                    } else if (key(i) > entry.key) {
                        if (n >= capacity()) {
                            return InsertResult::eFull;
                        }
                        // Shift the keys and values to make space for the new key.
                        setCount(n + 1);
                        shiftEntry(i);
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

            InsertResult insert(const Entry& entry) noexcept {
                if constexpr (kUseLowerBoundForLeafInsert) {
                    return insertViaLowerBound(entry);
                } else {
                    return insertViaScan(entry);
                }
            }

            void remove(size_t index) noexcept {
                auto keySet = keys();
                auto valueSet = values();
                std::move(keySet.begin() + index + 1, keySet.end(), keySet.begin() + index);
                std::move(valueSet.begin() + index + 1, valueSet.end(), valueSet.begin() + index);
                setCount(count() - 1);
            }

            size_t upperBound(const Key& k) const noexcept {
                auto keySet = keys();
                auto it = std::upper_bound(keySet.begin(), keySet.end(), k);
                return std::distance(keySet.begin(), it);
            }

            size_t lowerBound(const Key& k) const noexcept {
                auto keySet = keys();
                auto it = std::lower_bound(keySet.begin(), keySet.end(), k);
                return std::distance(keySet.begin(), it);
            }

            size_t indexOfScan(const Key& k) const noexcept {
                for (size_t i = start(); i < count(); i++) {
                    if (key(i) == k) {
                        return i;
                    }
                }
                return SIZE_MAX; // Key not found
            }

            size_t indexOfViaLowerBound(const Key& k) const noexcept {
                size_t index = lowerBound(k);
                if (index != count() && key(index) == k) {
                    return index; // Key found
                }
                return SIZE_MAX;
            }

            size_t indexOf(const Key& k) const noexcept {
                if constexpr (kUseLowerBoundForLeafFind) {
                    return indexOfViaLowerBound(k);
                } else {
                    return indexOfScan(k);
                }
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

            void setCount(size_t newCount) noexcept {
                KM_ASSERT(newCount <= capacity());
                mCount = newCount;
            }

            bool isFull() const noexcept {
                return count() == capacity();
            }

            bool isUnderFilled() const noexcept {
                return count() < leastCount();
            }

            bool isEmpty() const noexcept {
                return count() == 0;
            }

            constexpr size_t leastCount() const noexcept {
                return Leaf::minCapacity();
            }

            constexpr size_t capacity() const noexcept {
                return Leaf::maxCapacity();
            }

            constexpr static size_t maxCapacity() noexcept {
                return kOrder;
            }

            constexpr static size_t minCapacity() noexcept {
                return std::max(1zu, (maxCapacity() / 2) - 1);
            }

            const Key& minKey() const noexcept {
                KM_ASSERT(count() > 0);
                return key(0);
            }

            const Key& maxKey() const noexcept {
                KM_ASSERT(count() > 0);
                return key(count() - 1);
            }

            std::span<Key> keys() noexcept { return std::span(mKeys + start(), count()); }
            std::span<const Key> keys() const noexcept { return std::span(mKeys + start(), count()); }

            std::span<Value> values() noexcept { return std::span(mValues + start(), count()); }
            std::span<const Value> values() const noexcept { return std::span(mValues + start(), count()); }

            void claim(Leaf *other) noexcept {
                KM_ASSERT(other->isLeaf());
                KM_ASSERT(other->getParent() == this->getParent());

                size_t oldCount = count();

                // Move the keys and values from the other leaf to this leaf
                setCount(oldCount + other->count());

                {
                    auto thisKeys = keys();
                    auto thisValues = values();
                    auto otherKeys = other->keys();
                    auto otherValues = other->values();

                    std::move(otherKeys.begin(), otherKeys.end(), thisKeys.begin() + oldCount);
                    std::move(otherValues.begin(), otherValues.end(), thisValues.begin() + oldCount);
                }

                other->setCount(0);
            }

            /// @brief Split the internal node into two nodes to accommodate a new entry and leaf.
            void splitInto(Leaf *other, const Entry& entry, Entry *midpoint) noexcept {
                // guardrails
                KM_ASSERT(other->count() == 0);
                KM_ASSERT(this->count() == this->capacity());
                KM_ASSERT(this->find(entry.key) == nullptr);

                bool isOdd = (capacity() & 1);
                size_t half = capacity() / 2;
                size_t otherSize = half + (isOdd ? 1 : 0);

                Entry middle = {key(half), value(half)};

                // copy the top half of the keys and values to the new leaf
                other->setCount(otherSize);
                {
                    auto thisKeys = this->keys();
                    auto thisValues = this->values();
                    auto otherKeys = other->keys();
                    auto otherValues = other->values();

                    std::move(thisKeys.begin() + half, thisKeys.end(), otherKeys.begin());
                    std::move(thisValues.begin() + half, thisValues.end(), otherValues.begin());
                }
                setCount(half);

                if (entry.key < middle.key) {
                    this->insert(entry);
                    other->insert(middle);
                    *midpoint = this->popBack();
                } else {
                    other->insert(entry);
                    *midpoint = other->popFront();
                }

                KM_ASSERT(!other->isUnderFilled());
                KM_ASSERT(!this->isUnderFilled());
            }

            void validate() const noexcept {
                auto keySet = keys();
                bool isSorted = std::is_sorted(keySet.begin(), keySet.end());
                if (!isSorted) {
                    printf("Leaf node %p is not sorted\n", (void*)this);
                    for (size_t i = 0; i < count(); i++) {
                        printf("Key %zu: %d\n", i, (int)key(i));
                    }
                    KM_PANIC("Leaf node keys are not sorted");
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

            void dump(int depth) const {
                for (int i = 0; i < depth; i++) {
                    printf("  ");
                }
                printf("%p %d %zu: [ ", (void*)this, depth, count());
                for (size_t i = 0; i < count(); i++) {
                    printf("%d, ", (int)key(i));
                }
                printf("]\n");
            }
        };

        struct ChildEntry {
            Leaf *child;
            Entry entry;
        };

        struct Internal : public Leaf {
            using Super = Leaf;

        protected:
            Leaf *mChildren[Super::kOrder + 1]{};

            Internal(bool leaf, TreeNodeHeader *parent) noexcept
                : Super(leaf, parent)
            { }

        public:
            Internal(TreeNodeHeader *parent) noexcept
                : Internal(false, parent)
            { }

            using Super::count;
            using Super::setCount;
            using Super::capacity;
            using Super::key;
            using Super::value;
            using Super::emplace;
            using Super::minKey;
            using Super::maxKey;
            using Super::upperBound;
            using Super::start;
            using Super::keys;
            using Super::values;

            static constexpr size_t leafCapacity() noexcept {
                return Leaf::maxCapacity() + 1;
            }

            size_t leafCount() const noexcept {
                return count() + 1;
            }

            std::span<Leaf*> children() noexcept { return std::span(mChildren + start(), count() + 1); }
            std::span<Leaf* const> children() const noexcept { return std::span(mChildren + start(), count() + 1); }

            ChildEntry popBackChild() noexcept {
                KM_ASSERT(count() > 0);
                Entry entry = {key(count() - 1), value(count() - 1)};
                Leaf *leaf = child(count() - 1);
                setCount(count() - 1);
                return ChildEntry{leaf, entry};
            }

            /// @brief Returns the first element and the child node before it.
            ChildEntry popFrontChild() noexcept {
                KM_ASSERT(count() > 0);
                Entry entry = {key(0), value(0)};
                Leaf *leaf = child(0);

                auto keySet = keys();
                auto valueSet = values();
                auto childSet = children();

                std::move(keySet.begin() + 1, keySet.end(), keySet.begin());
                std::move(valueSet.begin() + 1, valueSet.end(), valueSet.begin());
                std::move(childSet.begin() + 1, childSet.end(), childSet.begin());

                setCount(count() - 1);

                return entry;
            }

            auto& child(this auto&& self, size_t index) noexcept {
                KM_ASSERT(index < self.count() + 1);
                return self.mChildren[index];
            }

            void takeChild(size_t index, Leaf *leaf) noexcept {
                leaf->setParent(this);
                child(index) = leaf;
            }

            Leaf *getLeaf(size_t index) const noexcept {
                return mChildren[index];
            }

            size_t findChildViaScan(const Key& key) const noexcept {
                auto keySet = keys();
                for (size_t i = 0; i < keySet.size(); i++) {
                    if (keySet[i] > key) {
                        return i;
                    }
                }

                return keySet.size(); // Return the last index if key is greater than all keys
            }

            size_t findChildViaUpperBound(const Key& k) const noexcept {
                return Super::upperBound(k);
            }

            size_t findChildIndex(const Key& key) const noexcept {
                if constexpr (kUseUpperBoundForInternalFind) {
                    return findChildViaUpperBound(key);
                } else {
                    return findChildViaScan(key);
                }
            }

            size_t indexOfChild(const Leaf *leaf) const noexcept {
                auto childSet = children();
                auto it = std::find(childSet.begin(), childSet.end(), leaf);
                KM_CHECK(it != childSet.end(), "Leaf not found in internal node");
                return std::distance(childSet.begin(), it);
            }

            void removeEntry(size_t index) noexcept {
                KM_ASSERT(index < count());
                auto keySet = keys();
                auto valueSet = values();
                auto childSet = children();

                std::move(keySet.begin() + index + 1, keySet.end(), keySet.begin() + index);
                std::move(valueSet.begin() + index + 1, valueSet.end(), valueSet.begin() + index);
                std::move(childSet.begin() + index + 1, childSet.end(), childSet.begin() + index);

                setCount(count() - 1);
            }

            void mergeLeafNodes(Leaf *lhs, Leaf *rhs) noexcept {
                KM_ASSERT(lhs->isLeaf() && rhs->isLeaf());
                KM_ASSERT(lhs->getParent() == this);
                KM_ASSERT(rhs->getParent() == this);

                size_t lhsIndex = indexOfChild(lhs);
                size_t rhsIndex = lhsIndex + 1;

                if constexpr (kTreeDebug) {
                    rhsIndex = indexOfChild(rhs);
                    KM_ASSERT((lhsIndex + 1) == rhsIndex); // Ensure they are adjacent
                }

                Entry middle = {key(lhsIndex), value(lhsIndex)};

                if (rhsIndex == count()) {
                    lhs->insert(middle);
                    lhs->claim(rhs);
                    setCount(count() - 1);
                } else {
                    lhs->insert(middle);
                    lhs->claim(rhs);

                    if (rhsIndex != count()) {
                        key(lhsIndex) = key(rhsIndex);
                        value(lhsIndex) = value(rhsIndex);
                    }

                    removeEntry(rhsIndex);
                }
            }

            void rebalanceIntoLowerLeafNode(Leaf *lhs, Leaf *rhs) noexcept {
                KM_ASSERT(lhs->isLeaf() && rhs->isLeaf());
                size_t lhsCount = lhs->count();
                size_t lhsIndex = indexOfChild(lhs);
                Entry middle = {key(lhsIndex), value(lhsIndex)};

                KM_ASSERT(lhsCount < Leaf::minCapacity());

                size_t itemsToMove = Leaf::minCapacity() - lhsCount;
                lhs->insert(middle);

                size_t newLhsCount = lhs->count();
                lhs->setCount(newLhsCount + itemsToMove);

                auto lhsKeySet = lhs->keys();
                auto lhsValueSet = lhs->values();
                auto rhsKeySet = rhs->keys();
                auto rhsValueSet = rhs->values();

                std::move(rhsKeySet.begin(), rhsKeySet.begin() + itemsToMove, lhsKeySet.begin() + newLhsCount);
                std::move(rhsValueSet.begin(), rhsValueSet.begin() + itemsToMove, lhsValueSet.begin() + newLhsCount);

                std::move(rhsKeySet.begin() + itemsToMove, rhsKeySet.end(), rhsKeySet.begin());
                std::move(rhsValueSet.begin() + itemsToMove, rhsValueSet.end(), rhsValueSet.begin());

                rhs->setCount(rhs->count() - itemsToMove);

                Entry newMiddle = lhs->popBack();
                key(lhsIndex) = newMiddle.key;
                value(lhsIndex) = newMiddle.value;
            }

            void rebalanceIntoUpperLeafNode(Leaf *lhs, Leaf *rhs) noexcept {
                KM_ASSERT(lhs->isLeaf() && rhs->isLeaf());
                size_t rhsCount = rhs->count();
                size_t rhsIndex = indexOfChild(rhs);
                Entry middle = {key(rhsIndex - 1), value(rhsIndex - 1)};

                size_t itemsToMove = Leaf::minCapacity() - rhsCount;
                rhs->insert(middle);

                {
                    rhs->setCount(rhs->count() + itemsToMove);

                    auto rhsKeySet = rhs->keys();
                    auto rhsValueSet = rhs->values();
                    auto lhsKeySet = lhs->keys();
                    auto lhsValueSet = lhs->values();

                    std::move_backward(rhsKeySet.begin(), rhsKeySet.end() - itemsToMove, rhsKeySet.end());
                    std::move_backward(rhsValueSet.begin(), rhsValueSet.end() - itemsToMove, rhsValueSet.end());

                    std::move(lhsKeySet.end() - itemsToMove, lhsKeySet.end(), rhsKeySet.begin());
                    std::move(lhsValueSet.end() - itemsToMove, lhsValueSet.end(), rhsValueSet.begin());

                    lhs->setCount(lhs->count() - itemsToMove);
                }

                Entry newMiddle = rhs->popFront();
                key(rhsIndex - 1) = newMiddle.key;
                value(rhsIndex - 1) = newMiddle.value;
            }

            void rebalanceLeafNodes(Leaf *lhs, Leaf *rhs) noexcept {
                KM_ASSERT(lhs->isLeaf() && rhs->isLeaf());
                size_t lhsCount = lhs->count();
                size_t rhsCount = rhs->count();

                if (lhsCount < rhsCount) {
                    rebalanceIntoLowerLeafNode(lhs, rhs);
                } else {
                    rebalanceIntoUpperLeafNode(lhs, rhs);
                }
            }

            template<mem::Allocator Allocator>
            void mergeInternalNodes(Internal *lhs, Internal *rhs, Allocator& allocator) noexcept {
                KM_ASSERT(!lhs->isLeaf() && !rhs->isLeaf());

                size_t lhsIndex = indexOfChild(lhs);
                size_t lhsCount = lhs->count();
                size_t rhsCount = rhs->count();

                KM_ASSERT(lhsCount + rhsCount >= Internal::minCapacity());

                Entry middle = {key(lhsIndex), value(lhsIndex)};
                lhs->claimInternal(middle, rhs);

                // shuffle the remaining internal nodes over one position
                auto keySet = keys();
                auto valueSet = values();
                auto childSet = children();
                std::move(keySet.begin() + lhsIndex + 1, keySet.end(), keySet.begin() + lhsIndex);
                std::move(valueSet.begin() + lhsIndex + 1, valueSet.end(), valueSet.begin() + lhsIndex);
                std::move(childSet.begin() + lhsIndex + 1 + 1, childSet.end(), childSet.begin() + lhsIndex + 1);

                setCount(count() - 1);

                BTreeMapCommon::releaseNodeMemory(rhs, allocator);
            }

            /// @brief Move the bottom N elements from rhs to lhs.
            void rebalanceIntoLowerInternalNode(Internal *lhs, Internal *rhs, size_t midpoint) noexcept {
                Entry middle = {key(midpoint), value(midpoint)};

                size_t lhsCount = lhs->count();

                size_t itemsToMove = Internal::minCapacity() - lhsCount;

                lhs->setCount(lhsCount + itemsToMove);

                auto lhsKeySet = lhs->keys();
                auto lhsValueSet = lhs->values();
                auto lhsChildSet = lhs->children();

                auto rhsKeySet = rhs->keys();
                auto rhsValueSet = rhs->values();
                auto rhsChildSet = rhs->children();

                // move the bottom N elements from rhs to lhs
                std::move(rhsKeySet.begin(), rhsKeySet.begin() + itemsToMove - 1, lhsKeySet.begin() + lhsCount);
                std::move(rhsValueSet.begin(), rhsValueSet.begin() + itemsToMove - 1, lhsValueSet.begin() + lhsCount);
                std::move(rhsChildSet.begin(), rhsChildSet.begin() + itemsToMove, lhsChildSet.begin() + lhsCount + 1);

                for (size_t i = lhsCount + 1; i < lhs->count() + 1; i++) {
                    lhsChildSet[i]->setParent(lhs);
                }

                lhs->emplace(lhsCount + itemsToMove - 1, middle);

                Entry newMiddle = {rhs->key(0), rhs->value(0)};

                // shrink rhs by the number of items moved and move its remaining keys and values down
                size_t remaining = rhs->count() - itemsToMove;
                std::move(rhsKeySet.begin() + itemsToMove, rhsKeySet.end(), rhsKeySet.begin());
                std::move(rhsValueSet.begin() + itemsToMove, rhsValueSet.end(), rhsValueSet.begin());
                std::move(rhsChildSet.begin() + itemsToMove, rhsChildSet.end(), rhsChildSet.begin());
                rhs->setCount(remaining);

                emplace(midpoint, newMiddle);
            }

            /// @brief Move the top N elements from lhs to rhs.
            void rebalanceIntoUpperInternalNode(Internal *lhs, Internal *rhs, size_t midpoint) noexcept {
                Entry middle = {key(midpoint), value(midpoint)};

                size_t rhsCount = rhs->count();

                size_t itemsToMove = Internal::minCapacity() - rhsCount;

                auto keySet = rhs->keys();
                auto valueSet = rhs->values();
                auto childSet = rhs->children();

                rhs->setCount(rhsCount + itemsToMove);

                std::move_backward(keySet.begin(), keySet.end(), keySet.end() + itemsToMove);
                std::move_backward(valueSet.begin(), valueSet.end(), valueSet.end() + itemsToMove);
                std::move_backward(childSet.begin(), childSet.end(), childSet.end() + itemsToMove);

                rhs->emplace(itemsToMove - 1, middle);

                size_t remaining = itemsToMove - 1;

                auto lhsKeySet = lhs->keys();
                auto lhsValueSet = lhs->values();
                auto lhsChildSet = lhs->children();

                auto rhsKeySet = rhs->keys();
                auto rhsValueSet = rhs->values();
                auto rhsChildSet = rhs->children();

                Entry newMiddle = {lhs->key(lhs->count() - 1), lhs->value(lhs->count() - 1)};

                std::move(lhsKeySet.end() - remaining, lhsKeySet.end(), rhsKeySet.begin());
                std::move(lhsValueSet.end() - remaining, lhsValueSet.end(), rhsValueSet.begin());
                std::move(lhsChildSet.end() - remaining - 1, lhsChildSet.end(), rhsChildSet.begin());

                for (size_t i = 0; i < itemsToMove; i++) {
                    rhsChildSet[i]->setParent(rhs);
                }

                lhs->setCount(lhs->count() - itemsToMove);

                emplace(midpoint, newMiddle);
            }

            void rebalanceInternalNodes(Internal *lhs, Internal *rhs) noexcept {
                KM_ASSERT(!lhs->isLeaf() && !rhs->isLeaf());

                size_t lhsIndex = indexOfChild(lhs);
                size_t lhsCount = lhs->count();
                size_t rhsCount = rhs->count();

                KM_ASSERT(lhsCount + rhsCount >= Internal::minCapacity() * 2);

                if (lhsCount < rhsCount) {
                    rebalanceIntoLowerInternalNode(lhs, rhs, lhsIndex);
                } else {
                    rebalanceIntoUpperInternalNode(lhs, rhs, lhsIndex);
                }
            }

            void claimInternal(const Entry& entry, Internal *other) noexcept {
                KM_ASSERT(!other->isLeaf());
                KM_ASSERT(other->getParent() == this->getParent());

                size_t n = count();
                setCount(n + other->count() + 1);
                emplace(n, entry);

                // Move the keys and values from the other internal node to this internal node
                {
                    auto keySet = keys();
                    auto valueSet = values();
                    auto otherKeySet = other->keys();
                    auto otherValueSet = other->values();

                    std::move(otherKeySet.begin(), otherKeySet.end(), keySet.begin() + n + 1);
                    std::move(otherValueSet.begin(), otherValueSet.end(), valueSet.begin() + n + 1);
                }

                {
                    auto childSet = children();
                    auto otherChildSet = other->children();

                    std::move(otherChildSet.begin(), otherChildSet.end(), childSet.begin() + n + 1);
                    for (size_t i = n + 1; i < count() + 1; i++) {
                        child(i)->setParent(this);
                    }
                }

                // Clear the other internal node
                other->setCount(0);
            }

            InsertResult insertChildViaLowerBound(const Entry& entry, Leaf *leaf) noexcept {
                size_t n = count();
                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                size_t index = Super::lowerBound(entry.key);
                if (index == n) {
                    setCount(n + 1);
                    emplace(n, entry);
                    takeChild(n + 1, leaf);
                    return InsertResult::eSuccess;
                } else {
                    KM_ASSERT(key(index) != entry.key);

                    // Shift the keys and values to make space for the new key.
                    setCount(n + 1);
                    Super::shiftEntry(index);
                    for (size_t j = n; j > index; j--) {
                        child(j + 1) = child(j);
                    }
                    emplace(index, entry);
                    takeChild(index + 1, leaf);
                    return InsertResult::eSuccess;
                }
            }

            InsertResult insertChildViaScan(const Entry& entry, Leaf *leaf) noexcept {
                // TODO: remove this isEmpty check once tests no longer violate the invariant
                if (!leaf->isEmpty()) {
                    KM_ASSERT(entry.key < readMinKey(leaf));
                }

                size_t n = count();

                if (n >= capacity()) {
                    return InsertResult::eFull;
                }

                for (size_t i = 0; i < n; i++) {
                    KM_ASSERT(key(i) != entry.key);

                    if (entry.key < key(i)) {
                        // Shift the keys and values to make space for the new key.
                        setCount(n + 1);
                        Super::shiftEntry(i);
                        for (size_t j = n; j > i; j--) {
                            child(j + 1) = child(j);
                        }
                        emplace(i, entry);
                        takeChild(i + 1, leaf);
                        return InsertResult::eSuccess;
                    }
                }

                // If we reach here, the key is greater than all existing keys.
                setCount(n + 1);
                emplace(n, entry);
                takeChild(n + 1, leaf);
                return InsertResult::eSuccess;
            }

            /// @brief Insert a new entry and the leaf node above it into the internal node.
            InsertResult insertChild(const Entry& entry, Leaf *leaf) noexcept {
                if constexpr (kUseLowerBoundForInternalInsert) {
                    return insertChildViaLowerBound(entry, leaf);
                } else {
                    return insertChildViaScan(entry, leaf);
                }
            }

            void transferInto(Internal *other, Entry *midpoint) noexcept {
                // 512: (capacity() = 7, half = 3)
                // 256: (capacity() = 15, half = 7)
                // 128: (capacity() = 30, half = 15)
                bool isOdd = (capacity() & 1);
                size_t half = capacity() / 2;
                Entry middle = {key(half), value(half)};

                other->setCount(half - (isOdd ? 0 : 1));

                {
                    auto keySet = keys();
                    auto valueSet = values();
                    auto otherKeySet = other->keys();
                    auto otherValueSet = other->values();

                    std::move(keySet.begin() + half + 1, keySet.end(), otherKeySet.begin());
                    std::move(valueSet.begin() + half + 1, valueSet.end(), otherValueSet.begin());
                }

                {
                    auto childSet = children();
                    auto otherChildSet = other->children();
                    std::move(childSet.begin() + half + 1, childSet.end(), otherChildSet.begin());
                    for (auto *child : otherChildSet) {
                        child->setParent(other);
                    }
                }

                setCount(half);
                *midpoint = middle;

                KM_ASSERT(!other->containsInNode(middle.key));
            }

            void splitIntoUpperHalf(Internal *other, const ChildEntry& entry, Entry *midpoint) noexcept {
                /// Divide this node into another, assuming that the entry is greater than the midpoint
                /// | 1 | 2 | 3 | 4 | 5 |
                ///            ^^^^^^^^^^ entry is within this area
                /// Extract the middle node into a local
                /// Retain first M/2 entries and (M/2)+1 leaves and transfer the remaining to the other node.
                /// Then insert the new entry and leaf into the other node

                transferInto(other, midpoint);
                other->insertChild(entry.entry, entry.child);
            }

            void splitIntoLowerHalf(Internal *other, const ChildEntry& entry, Entry *midpoint) noexcept {
                transferInto(other, midpoint);
                this->insertChild(entry.entry, entry.child);
            }

            /// @brief Split the internal node into two nodes to accommodate a new entry and leaf.
            void splitInternalNode(Internal *other, const ChildEntry& entry, Entry *midpoint) noexcept {
                // guardrails
                KM_ASSERT(other->count() == 0);
                KM_ASSERT(this->count() == this->capacity());
                KM_ASSERT(!this->isLeaf());
                KM_ASSERT(!other->isLeaf());

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
                setCount(1);
                takeChild(0, lhs);
                takeChild(1, rhs);
                Super::emplace(0, entry);
            }

            template<mem::Allocator Allocator>
            void destroyChildren(Allocator& allocator) noexcept {
                for (size_t i = 0; i < count() + 1; i++) {
                    BTreeMapCommon::deleteNode(child(i), allocator);
                }
            }

            void validate() const noexcept {
                auto keySet = Super::keys();
                bool sorted = std::is_sorted(keySet.begin(), keySet.end());
                if (!sorted) {
                    printf("Internal node %p is not sorted\n", (void*)this);
                    for (size_t i = 0; i < count(); i++) {
                        printf("Key %zu: %d\n", i, (int)key(i));
                    }
                    KM_PANIC("Internal node keys are not sorted");
                }

                for (size_t i = 0; i < count() + 1; i++) {
                    Leaf *childNode = child(i);
                    if (childNode == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i, (void*)this);
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                    if (childNode->getParent() != this) {
                        printf("Child node %p has parent %p, expected %p\n", (void*)childNode, (void*)childNode->getParent(), (void*)this);
                        KM_PANIC("Child node parent mismatch in BTreeMap");
                    }

                    if (childNode->isLeaf()) {
                        static_cast<const Leaf*>(childNode)->validate();
                    } else {
                        static_cast<const Internal*>(childNode)->validate();
                    }
                }

                for (size_t i = 0; i < count(); i++) {
                    Leaf *current = child(i);
                    Leaf *next = child(i + 1);
                    for (size_t j = 0; j < current->count(); j++) {
                        if (!(current->key(j) < Super::key(i))) {
                            dump(0);
                            printf("Key %zu: %d, expected < %d\n", j, (int)current->key(j), (int)Super::key(i));
                        }
                        KM_ASSERT(current->key(j) < Super::key(i));
                    }
                    for (size_t j = 0; j < next->count(); j++) {
                        if (!(Super::key(i) < next->key(j))) {
                            dump(0);
                            printf("Key %zu: %d, expected > %d\n", j, (int)next->key(j), (int)Super::key(i));
                        }
                        KM_ASSERT(Super::key(i) < next->key(j));
                    }
                }
            }

            void dump(int depth) const {
                for (int i = 0; i < depth; i++) {
                    printf("  ");
                }
                printf("%p %d %zu: [ ", (void*)this, depth, count());
                for (size_t i = 0; i < count(); i++) {
                    printf("%d, ", (int)key(i));
                }
                printf("]\n");

                for (size_t i = 0; i < count() + 1; i++) {
                    Leaf *inner = child(i);
                    if (inner->isLeaf()) {
                        static_cast<const Leaf*>(inner)->dump(depth + 1);
                    } else {
                        static_cast<const Internal*>(inner)->dump(depth + 1);
                    }
                }
            }
        };

        /// @brief Tracks the number of internal and leaf nodes that will need to be split if
        ///        the bottom node is full.
        ///        This is used to provide resilience against OOM ocurring during insertions.
        class InsertMemoryTracker {
            size_t mFullInternalNodes;
            void *mNodes;

            void *drainNode() noexcept {
                if (mNodes == nullptr) {
                    return nullptr;
                }

                void *current = mNodes;
                void *next = *(void**)current;
                mNodes = next;
                return current;
            }

        public:
            constexpr InsertMemoryTracker() noexcept
                : mFullInternalNodes(0)
                , mNodes(nullptr)
            { }

            void track(Internal *node) noexcept {
                if (node->isFull()) {
                    if (node->isRootNode()) {
                        // Splitting the root node requires an extra allocation for the new root
                        mFullInternalNodes += 2;
                    } else {
                        mFullInternalNodes += 1;
                    }
                }
            }

            template<mem::Allocator Allocator>
            OsStatus allocateMemory(Allocator& allocator) noexcept {
                for (size_t i = 0; i < mFullInternalNodes; i++) {
                    void *node = allocator.allocateAligned(sizeof(Internal), alignof(Internal));
                    if (node == nullptr) {
                        releaseRemaining(allocator);
                        return OsStatusOutOfMemory;
                    }

                    *(void**)node = mNodes;
                    mNodes = node;
                }

                return OsStatusSuccess;
            }

            template<typename... A>
            [[gnu::returns_nonnull]]
            Internal *takeNode(A&&... args) noexcept {
                void *ptr = drainNode();
                if (ptr == nullptr) {
                    printf("Ran out of memory in tracker with %zu nodes\n", mFullInternalNodes);
                }
                KM_ASSERT(ptr != nullptr);

                return new (ptr) Internal(std::forward<A>(args)...);
            }

            template<mem::Allocator Allocator>
            void releaseRemaining(Allocator& allocator) {
                while (void *node = drainNode()) {
                    allocator.deallocate(node, sizeof(Internal));
                }
            }
        };

        template<mem::Allocator Allocator>
        static void rebalanceOrMergeInternal(Internal *lhs, Internal *rhs, Allocator& allocator) noexcept {
            KM_ASSERT(!lhs->isLeaf() && !rhs->isLeaf());
            KM_ASSERT(lhs->getParent() == rhs->getParent());

            if (!lhs->isUnderFilled() && !rhs->isUnderFilled()) {
                // Neither internal node is underfilled, no action needed
                return;
            }

            Internal *parent = static_cast<Internal*>(lhs->getParent());
            size_t lhsCount = lhs->count();
            size_t rhsCount = rhs->count();

            bool canRebalance = (lhsCount + rhsCount) >= (Internal::minCapacity() * 2);

            if (!canRebalance) {
                parent->mergeInternalNodes(lhs, rhs, allocator);
            } else {
                parent->rebalanceInternalNodes(lhs, rhs);
            }
        }

        template<mem::Allocator Allocator>
        static void rebalanceOrMergeLeaf(Leaf *lhs, Leaf *rhs, Allocator& allocator) noexcept {
            KM_ASSERT(lhs->isLeaf() && rhs->isLeaf());
            KM_ASSERT(lhs->getParent() == rhs->getParent());

            if (!lhs->isUnderFilled() && !rhs->isUnderFilled()) {
                // Neither leaf is underfilled, no action needed
                return;
            }

            Internal *parent = static_cast<Internal*>(lhs->getParent());
            size_t lhsCount = lhs->count();
            size_t rhsCount = rhs->count();

            KM_ASSERT(lhsCount + rhsCount >= Leaf::minCapacity());

            // Check if we can rebalance the leaves
            bool canRebalance = (lhsCount + rhsCount) >= (Leaf::minCapacity() * 2);

            if (!canRebalance) {
                // Merge into the left node and delete the right node
                parent->mergeLeafNodes(lhs, rhs);
                BTreeMapCommon::deleteNode(rhs, allocator);
            } else {
                parent->rebalanceLeafNodes(lhs, rhs);
            }
        }

        static void splitNode(Leaf *node, Leaf *newNode, const Entry& entry, Entry *midpoint) noexcept {
            KM_ASSERT(node->isLeaf() == newNode->isLeaf());

            if (node->isLeaf()) {
                node->splitInto(newNode, entry, midpoint);
            } else {
                // TODO: this branch has no coverage
                // it is invoked by splitRootNode but it would seem that the root node is never split
                // while its an internal node, which doesnt make sense.
                Internal *internalNode = static_cast<Internal*>(node);
                Internal *otherInternal = static_cast<Internal*>(newNode);
                internalNode->splitInto(otherInternal, entry, midpoint);
            }
        }

        template<mem::Allocator Allocator>
        static void deleteNode(TreeNodeHeader *node, Allocator& allocator) noexcept {
            if (!node->isLeaf()) {
                Internal *internal = static_cast<Internal*>(node);
                internal->destroyChildren(allocator);
            }

            releaseNodeMemory(node, allocator);
        }

        template<mem::Allocator Allocator>
        static void releaseNodeMemory(TreeNodeHeader *node, Allocator& allocator) noexcept {
            bool isLeaf = node->isLeaf();
            Leaf *leaf = static_cast<Leaf*>(node);
            std::destroy_at(leaf);

            allocator.deallocate(static_cast<void*>(node), isLeaf ? sizeof(Leaf) : sizeof(Internal));
        }

        static Key readMinKey(const Leaf *node) noexcept {
            if (node->isLeaf()) {
                return node->minKey();
            } else {
                const Internal *internal = static_cast<const Internal*>(node);
                return readMinKey(internal->child(0));
            }
        }
    };

    template<typename Key, typename Value>
    using TreeNodeLeaf = BTreeMapCommon<Key, Value>::Leaf;

    template<typename Key, typename Value>
    using TreeNodeInternal = BTreeMapCommon<Key, Value>::Internal;

    template<typename Key, typename Value>
    void dumpNode(const TreeNodeLeaf<Key, Value> *node) noexcept {
        if (node->isLeaf()) {
            static_cast<const TreeNodeLeaf<Key, Value>*>(node)->dump();
        } else {
            static_cast<const TreeNodeInternal<Key, Value>*>(node)->dump();
        }
    }

    template<typename Key, typename Value>
    Key readMinKey(const TreeNodeLeaf<Key, Value> *node) noexcept {
        return BTreeMapCommon<Key, Value>::readMinKey(node);
    }
}

namespace sm {
    struct BTreeStats {
        size_t leafCount = 0;
        size_t internalNodeCount = 0;
        size_t depth = 0;
        size_t memoryUsage = 0;
    };

    template<typename Key, typename Value>
    class BTreeMapIterator {
        using Common = detail::BTreeMapCommon<Key, Value>;
        using Entry = typename Common::Entry;
        using InternalNode = typename Common::Internal;
        using LeafNode = typename Common::Leaf;

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
                mCurrentNode = childNode;
                while (!mCurrentNode->isLeaf()) {
                    InternalNode *newNode = static_cast<InternalNode*>(static_cast<InternalNode*>(mCurrentNode)->child(0));
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
            return {key, value};
        }

        std::pair<const Key&, const Value&> operator*() const noexcept {
            auto& key = mCurrentNode->key(mCurrentIndex);
            auto& value = mCurrentNode->value(mCurrentIndex);
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

        size_t getCurrentNodeIndex() const noexcept {
            return mCurrentIndex;
        }

        LeafNode *getCurrentNode() const noexcept {
            return mCurrentNode;
        }
    };

    /// @brief B-Tree Map.
    ///
    /// @tparam Key The type of the keys in the map.
    /// @tparam Value The type of the values in the map.
    /// @tparam Compare The comparison function for keys, must be a stateless default constructible object.
    /// @tparam Equal The equality function for keys, must be a stateless default constructible object.
    /// @tparam Allocator The allocator to use for memory management.
    template<
        typename Key,
        typename Value,
        typename Compare = std::less<Key>,
        typename Equal = std::equal_to<Key>,
        mem::Allocator Allocator = mem::GenericAllocator
    >
    class BTreeMap {
        using Common = detail::BTreeMapCommon<Key, Value>;
        using Entry = typename Common::Entry;
        using ChildEntry = typename Common::ChildEntry;
        using InternalNode = typename Common::Internal;
        using LeafNode = typename Common::Leaf;
        using InsertMemoryTracker = typename Common::InsertMemoryTracker;
        using MutIterator = BTreeMapIterator<Key, Value>;

        [[no_unique_address]] Allocator mAllocator;
        [[no_unique_address]] Compare mCompare{};
        [[no_unique_address]] Equal mEqual{};
        LeafNode *mRootNode;

        template<typename T, typename... Args>
        T *allocateNode(Args&&... args) noexcept {
            void *ptr = mAllocator.allocateAligned(sizeof(T), alignof(T));
            if (ptr == nullptr) {
                return nullptr;
            }
            return new (ptr) T(std::forward<Args>(args)...);
        }

        template<typename T>
        void destroyNode(T *node) noexcept {
            Common::deleteNode(node, mAllocator);
        }

        OsStatus splitRootNode(LeafNode *leaf, const Entry& entry) noexcept {
            Entry midpoint;
            bool isLeaf = leaf->isLeaf();
            InternalNode *newRoot = allocateNode<InternalNode>(nullptr);
            LeafNode *newLeaf = isLeaf ? allocateNode<LeafNode>(newRoot) : allocateNode<InternalNode>(newRoot);

            if (newRoot == nullptr || newLeaf == nullptr) {
                if (newRoot) {
                    destroyNode(newRoot);
                }

                if (newLeaf) {
                    destroyNode(newLeaf);
                }

                return OsStatusOutOfMemory;
            }

            Common::splitNode(leaf, newLeaf, entry, &midpoint);

            newRoot->initAsRoot(leaf, newLeaf, midpoint);
            mRootNode = newRoot;

            return OsStatusSuccess;
        }

        OsStatus splitLeafNode(LeafNode *leaf, const Entry& entry, InsertMemoryTracker& tracker) noexcept {
            Entry midpoint;
            bool isLeaf = leaf->isLeaf();
            InternalNode *parent = static_cast<InternalNode*>(leaf->getParent());

            if (OsStatus status = tracker.allocateMemory(mAllocator)) {
                return status;
            }

            LeafNode *newLeaf = isLeaf ? allocateNode<LeafNode>(parent) : allocateNode<InternalNode>(parent);
            if (newLeaf == nullptr) {
                tracker.releaseRemaining(mAllocator);
                return OsStatusOutOfMemory;
            }

            Common::splitNode(leaf, newLeaf, entry, &midpoint);

            insertNewLeaf(parent, newLeaf, midpoint, tracker);

            tracker.releaseRemaining(mAllocator);

            return OsStatusSuccess;
        }

        OsStatus splitNode(LeafNode *leaf, const Entry& entry, InsertMemoryTracker& tracker) noexcept {
            // Splitting the root node is a special case.
            if (leaf == mRootNode) {
                return splitRootNode(leaf, entry);
            } else {
                return splitLeafNode(leaf, entry, tracker);
            }
        }

        void splitInternalNode(InternalNode *parent, const Entry& entry, LeafNode *leaf, InsertMemoryTracker& tracker) noexcept {
            InternalNode *newNode = tracker.takeNode(parent->getParent());

            Entry midpoint;
            parent->splitInternalNode(newNode, ChildEntry{leaf, entry}, &midpoint);

            if (parent == mRootNode) {
                InternalNode *newRoot = tracker.takeNode(nullptr);
                newRoot->initAsRoot(parent, newNode, midpoint);
                mRootNode = newRoot;
            } else {
                insertNewLeaf(static_cast<InternalNode*>(parent->getParent()), newNode, midpoint, tracker);
            }
        }

        /// @brief Insert a new node into an internal node.
        ///
        /// @param parent The parent internal node where the new leaf will be inserted.
        /// @param newLeaf The new leaf node to be inserted.
        void insertNewLeaf(InternalNode *parent, LeafNode *newLeaf, const Entry& midpoint, InsertMemoryTracker& tracker) noexcept {
            detail::InsertResult result = parent->insertChild(midpoint, newLeaf);
            if (result == detail::InsertResult::eFull) {
                splitInternalNode(parent, midpoint, newLeaf, tracker);
            }
        }

        OsStatus insertInto(LeafNode *node, const Entry& entry, InsertMemoryTracker& tracker) noexcept {
            if (node->isLeaf()) {
                detail::InsertResult result = node->insert(entry);
                if (result == detail::InsertResult::eFull) {
                    return splitNode(node, entry, tracker);
                }
                return OsStatusSuccess;
            } else {
                InternalNode *internal = static_cast<InternalNode*>(node);
                size_t index = internal->upperBound(entry.key);

                if (index != 0) {
                    if (internal->key(index - 1) == entry.key) {
                        internal->value(index - 1) = entry.value; // Update existing key
                        return OsStatusSuccess;
                    }
                }

                tracker.track(internal);

                KM_ASSERT(internal->find(entry.key) == nullptr);
                LeafNode *child = internal->child(index);
                return insertInto(child, entry, tracker);
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

        void rebalanceInnerLeafNode(LeafNode *node) noexcept {
            InternalNode *parent = static_cast<InternalNode*>(node->getParent());
            size_t index = parent->indexOfChild(node);

            LeafNode *adjacentNode;
            if (index == 0) {
                adjacentNode = parent->getLeaf(1);
            } else {
                LeafNode *tmp = parent->getLeaf(index - 1);
                adjacentNode = node;
                node = tmp;
            }

            Common::rebalanceOrMergeLeaf(node, adjacentNode, mAllocator);
        }

        void rebalanceInnerInternalNode(InternalNode *node) noexcept {
            InternalNode *parent = static_cast<InternalNode*>(node->getParent());
            size_t index = parent->indexOfChild(node);

            InternalNode *adjacentNode;
            if (index == 0) {
                adjacentNode = static_cast<InternalNode*>(parent->getLeaf(1));
            } else {
                InternalNode *tmp = static_cast<InternalNode*>(parent->getLeaf(index - 1));
                adjacentNode = node;
                node = tmp;
            }

            Common::rebalanceOrMergeInternal(node, adjacentNode, mAllocator);
        }

        LeafNode *eraseFromRootLeafNode(MutIterator it) {
            LeafNode *node = it.getCurrentNode();
            size_t index = it.getCurrentNodeIndex();

            KM_ASSERT(node->isRootNode() && node->isLeaf());

            node->remove(index);

            return node;
        }

        LeafNode *eraseFromRootInternalNode(MutIterator it) {
            LeafNode *node = it.getCurrentNode();
            KM_ASSERT(node->isRootNode() && !node->isLeaf());

            InternalNode *internal = static_cast<InternalNode*>(node);
            size_t index = it.getCurrentNodeIndex();
            LeafNode *child = internal->getLeaf(index);
            Promotion promoted = promoteBack(child);
            internal->emplace(index, promoted.entry);

            return promoted.leaf;
        }

        struct Promotion {
            Entry entry;
            LeafNode *leaf;
        };

        Promotion promoteBack(LeafNode *node) {
            KM_ASSERT(node->count() > 0);
            if (node->isLeaf()) {
                return Promotion { node->popBack(), node };
            } else {
                InternalNode *internal = static_cast<InternalNode*>(node);
                Promotion promoted = promoteBack(internal->getLeaf(internal->count()));
                return promoted;
            }
        }

        LeafNode *eraseFromRootNode(MutIterator it) {
            LeafNode *node = it.getCurrentNode();
            KM_ASSERT(node->isRootNode());
            if (node->isLeaf()) {
                return eraseFromRootLeafNode(it);
            } else {
                return eraseFromRootInternalNode(it);
            }
        }

        LeafNode *eraseFromInnerLeafNode(MutIterator it) {
            LeafNode *node = it.getCurrentNode();
            size_t index = it.getCurrentNodeIndex();
            KM_ASSERT(!node->isRootNode() && node->isLeaf());
            node->remove(index);

            return node;
        }

        LeafNode *eraseFromInnerInternalNode(MutIterator it) {
            LeafNode *node = it.getCurrentNode();
            KM_ASSERT(!node->isRootNode() && !node->isLeaf());

            InternalNode *internal = static_cast<InternalNode*>(node);
            size_t index = it.getCurrentNodeIndex();
            LeafNode *child = internal->getLeaf(index);
            Promotion promoted = promoteBack(child);
            internal->emplace(index, promoted.entry);

            return promoted.leaf;
        }

        LeafNode *eraseFromInnerNode(MutIterator it) {
            LeafNode *node = it.getCurrentNode();
            KM_ASSERT(!node->isRootNode());
            if (node->isLeaf()) {
                return eraseFromInnerLeafNode(it);
            } else {
                return eraseFromInnerInternalNode(it);
            }
        }

        LeafNode *eraseNodeElement(MutIterator it) {
            LeafNode *node = it.getCurrentNode();
            if (node->isRootNode()) {
                return eraseFromRootNode(it);
            } else {
                return eraseFromInnerNode(it);
            }
        }

        void eraseFromNode(MutIterator it) {
            LeafNode *node = eraseNodeElement(it);

            KM_ASSERT(node->isLeaf());
            if (!node->isRootNode()) {
                InternalNode *parent = static_cast<InternalNode*>(node->getParent());
                rebalanceInnerLeafNode(node);
                mergeRecursive(parent);
            } else {
                mergeRootNodeIfNeeded(node);
            }
        }

        bool mergeRootNodeIfNeeded(LeafNode *node) noexcept {
            if (node->count() == 0) {
                if (node->isLeaf()) {
                    // root node is empty, delete it
                    KM_ASSERT(mRootNode == node);
                    mRootNode = nullptr;
                    Common::releaseNodeMemory(node, mAllocator);
                } else {
                    InternalNode *internal = static_cast<InternalNode*>(node);
                    KM_ASSERT(internal->count() == 0);
                    KM_ASSERT(internal->child(0) != nullptr);
                    LeafNode *child = internal->child(0);
                    child->setParent(nullptr);
                    mRootNode = child;
                    Common::releaseNodeMemory(internal, mAllocator);
                }

                return true;
            } else if (node->count() == 1 && !node->isLeaf()) {
                InternalNode *internal = static_cast<InternalNode*>(node);
                LeafNode *lhs = internal->child(0);
                LeafNode *rhs = internal->child(1);

                KM_ASSERT(lhs->isLeaf() == rhs->isLeaf());
                bool leafChildren = lhs->isLeaf() && rhs->isLeaf();

                if (leafChildren) {
                    size_t lhsCount = lhs->count();
                    size_t rhsCount = rhs->count();
                    bool canMerge = (lhsCount + rhsCount) < LeafNode::maxCapacity();
                    if (canMerge) {
                        internal->mergeLeafNodes(lhs, rhs);
                    } else {
                        return false;
                    }
                } else {
                    InternalNode *lhsInternal = static_cast<InternalNode*>(lhs);
                    InternalNode *rhsInternal = static_cast<InternalNode*>(rhs);
                    size_t lhsCount = lhsInternal->leafCount();
                    size_t rhsCount = rhsInternal->leafCount();
                    bool canMerge = (lhsCount + rhsCount) <= InternalNode::leafCapacity();

                    if (canMerge) {
                        internal->mergeInternalNodes(lhsInternal, rhsInternal, mAllocator);
                    } else {
                        return false;
                    }
                }

                lhs->setParent(nullptr);
                mRootNode = lhs;
                Common::releaseNodeMemory(internal, mAllocator);

                if (leafChildren) {
                    Common::releaseNodeMemory(rhs, mAllocator);
                }

                return true;
            }

            return false;
        }

        void mergeRecursive(InternalNode *parent) noexcept {
            while (parent != nullptr) {
                parent = mergeInnerNodeIfNeeded(parent);
            }
        }

        InternalNode *mergeInnerNodeIfNeeded(InternalNode *node) noexcept {
            if (!node->isUnderFilled()) {
                return static_cast<InternalNode*>(node->getParent()); // Node is not underfilled, no action needed
            }

            if (node->isRootNode()) {
                mergeRootNodeIfNeeded(node);
                return nullptr; // No parent to return to
            }

            InternalNode *parent = static_cast<InternalNode*>(node->getParent());

            rebalanceInnerInternalNode(static_cast<InternalNode*>(node));

            return parent;
        }

        MutIterator findInNode(const Key& key) noexcept {
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

        void dumpRootNode() const {
            auto dumpNode = [&](this auto&& self, const LeafNode *node, int currentDepth) {
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
                    const InternalNode *internal = static_cast<const InternalNode*>(node);
                    printf("%p %d %zu: [", (void*)internal, currentDepth, internal->count());
                    for (size_t i = 0; i < internal->count(); i++) {
                        printf("%d, ", (int)internal->key(i));
                    }
                    printf("]\n");
                    for (size_t i = 0; i < internal->count() + 1; i++) {
                        self(internal->getLeaf(i), currentDepth + 1);
                    }
                }
            };

            dumpNode(mRootNode, 0);
        }

        void validateRootNode() const noexcept {
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
                               (void*)node, currentDepth, node->count(), lowerBound, node->capacity());

                        dump();
                    }
                    KM_ASSERT(node->count() >= lowerBound);
                }

                if (node->isLeaf()) {
                    if (currentDepth != depth) {
                        printf("Leaf node %p at depth %zu does not match expected depth %zu\n",
                               (void*)node, currentDepth, depth);
                        dump();
                    }
                    KM_ASSERT(currentDepth == depth);
                    return;
                }

                const InternalNode *internalNode = static_cast<const InternalNode*>(node);

                for (size_t i = 0; i < internalNode->count(); i++) {
                    LeafNode *prev = internalNode->getLeaf(i);
                    LeafNode *next = internalNode->getLeaf(i + 1);
                    if (prev == nullptr || next == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i, (void*)internalNode);
                        this->dump();
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                    for (size_t j = 0; j < prev->count(); j++) {
                        if (prev->key(j) >= internalNode->key(i)) {
                            printf("Key %d in previous node %p at depth %zu is not less than key %d in internal node %p\n",
                                   (int)prev->key(j), (void*)prev, currentDepth, (int)internalNode->key(i), (void*)internalNode);

                            dump();
                        }
                        KM_ASSERT(prev->key(j) < internalNode->key(i));
                    }

                    for (size_t j = 0; j < next->count(); j++) {
                        if (internalNode->key(i) >= next->key(j)) {
                            printf("Key %d in internal node %p at depth %zu is not less than key %d in next node %p\n",
                                   (int)internalNode->key(i), (void*)internalNode, currentDepth, (int)next->key(j), (void*)next);
                            dump();
                        }
                        KM_ASSERT(internalNode->key(i) < next->key(j));
                    }
                }

                for (size_t i = 0; i < internalNode->count() + 1; i++) {
                    LeafNode *child = internalNode->getLeaf(i);
                    if (child == nullptr) {
                        printf("Child node at index %zu is null in internal node %p\n", i, (void*)internalNode);
                        this->dump();
                        KM_PANIC("Child node is null in BTreeMap");
                    }
                    if (child->getParent() != internalNode) {
                        printf("Child node %p has parent %p, expected %p\n", (void*)child, (void*)child->getParent(), (void*)internalNode);
                        this->dump();
                        KM_PANIC("Child node parent mismatch in BTreeMap");
                    }
                    KM_ASSERT(child->getParent() == internalNode);
                    validateNode(child, currentDepth + 1);
                }
                validateNode(internalNode->getLeaf(internalNode->count()), currentDepth + 1); // Last child
            };

            validateNode(mRootNode, 1);
        }

        size_t countElements() const noexcept {
            size_t count = 0;

            auto countNode = [&](this auto&& self, const LeafNode *node) -> void {
                count += node->count();
                if (!node->isLeaf()) {
                    const InternalNode *internal = static_cast<const InternalNode*>(node);
                    for (size_t i = 0; i < internal->count() + 1; i++) {
                        self(internal->getLeaf(i));
                    }
                }
            };

            countNode(mRootNode);
            return count;
        }

        void gatherStats(LeafNode *node, BTreeStats &stats, size_t currentDepth) const noexcept {
            if (node->isLeaf()) {
                stats.leafCount += 1;
                stats.memoryUsage += sizeof(LeafNode);
            } else {
                stats.internalNodeCount += 1;
                stats.memoryUsage += sizeof(InternalNode);
            }

            stats.depth = std::max(stats.depth, currentDepth);
            if (node->isLeaf()) {
                return; // Reached a leaf node, no further children
            }

            const InternalNode *internal = static_cast<const InternalNode*>(node);
            for (size_t i = 0; i < internal->count() + 1; i++) {
                LeafNode *child = internal->getLeaf(i);
                gatherStats(child, stats, currentDepth + 1);
            }
        }

    public:
        using Iterator = MutIterator;

        constexpr BTreeMap() noexcept
            : mAllocator(Allocator{})
            , mRootNode(nullptr)
        { }

        constexpr BTreeMap(Allocator allocator) noexcept
            : mAllocator(std::move(allocator))
            , mRootNode(nullptr)
        { }

        constexpr BTreeMap(BTreeMap&& other) noexcept
            : mAllocator(std::move(other.mAllocator))
            , mCompare(std::move(other.mCompare))
            , mEqual(std::move(other.mEqual))
            , mRootNode(other.mRootNode)
        {
            other.mRootNode = nullptr;
        }

        constexpr BTreeMap& operator=(BTreeMap&& other) noexcept {
            if (this != &other) {
                clear();
                mAllocator = std::move(other.mAllocator);
                mCompare = std::move(other.mCompare);
                mEqual = std::move(other.mEqual);
                mRootNode = other.mRootNode;
                other.mRootNode = nullptr;
            }
            return *this;
        }

        UTIL_NOCOPY(BTreeMap);

        ~BTreeMap() noexcept {
            clear();
        }

        OsStatus insert(const Key& key, const Value& value) noexcept {
            if (mRootNode == nullptr) {
                mRootNode = allocateNode<LeafNode>(nullptr);
            }

            if (mRootNode == nullptr) {
                return OsStatusOutOfMemory;
            }

            InsertMemoryTracker tracker;
            return insertInto(mRootNode, Entry{key, value}, tracker);
        }

        void remove(const Key& key) noexcept {
            erase(find(key));
        }

        void erase(Iterator it) noexcept {
            if (it == end()) {
                return; // Nothing to erase
            }

            eraseFromNode(it);
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

            return findInNode(key);
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

        size_t count() const noexcept {
            if (mRootNode == nullptr) {
                return 0;
            }

            return countElements();
        }

        bool isEmpty() const noexcept {
            return mRootNode == nullptr;
        }

        void clear() noexcept {
            if (mRootNode) {
                destroyNode(mRootNode);
                mRootNode = nullptr;
            }
        }

        void dump() const noexcept {
            if (mRootNode == nullptr) {
                printf("BTreeMap is empty\n");
                return;
            }

            dumpRootNode();
        }

        void validate() const noexcept {
            if (!mRootNode) {
                return; // Empty tree is valid
            }

            validateRootNode();
        }

        BTreeStats stats() const noexcept {
            BTreeStats stats{};
            if (mRootNode) {
                gatherStats(mRootNode, stats, 0);
            }
            return stats;
        }
    };
}
