#pragma once

#include "std/container/detail/btree.hpp"

#include "panic.hpp"

#include "util/memory.hpp"

#include <stdlib.h>

namespace sm::detail {
    template<typename Key, typename Value>
    struct BTreeMapCommon {
        static constexpr size_t kTargetSize = sm::kilobytes(4).bytes();

        class Base;
        class Leaf;
        class Internal;

        struct Entry {
            Key key;
            Value value;
        };

        class Base {
            bool mIsLeaf;
            uint16_t mCount;
            Internal *mParent;

        public:
            Base(bool isLeaf, Internal *parent) noexcept
                : mIsLeaf(isLeaf)
                , mCount(0)
                , mParent(parent)
            { }

            virtual ~Base() noexcept = default;

            virtual size_t capacity() const noexcept = 0;
            virtual void dump(int depth) const noexcept = 0;
            virtual void validate() const noexcept = 0;

            bool isFull() const noexcept {
                return count() == capacity();
            }

            bool isEmpty() const noexcept {
                return count() == 0;
            }

            size_t count() const noexcept {
                return mCount;
            }

            void setCount(size_t count) noexcept {
                mCount = static_cast<uint16_t>(count);
            }

            bool isLeaf() const noexcept {
                return mIsLeaf;
            }

            Internal *getParent() const noexcept {
                return mParent;
            }

            void setParent(Internal *parent) noexcept {
                mParent = parent;
            }
        };

        static constexpr size_t kLeafOrder = std::max(3zu, (kTargetSize - sizeof(Base)) / (sizeof(Key) + sizeof(Value)));

        class Leaf : public Base {
            Key mKeys[kLeafOrder];
            Value mValues[kLeafOrder];

        public:
            using Base::count;
            using Base::setCount;
            using Base::isFull;
            using Base::isEmpty;

            Leaf(Internal *parent) noexcept
                : Base(true, parent)
            { }

            size_t capacity() const noexcept override {
                return maxCapacity();
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
                auto keySet = keys();
                bool isSorted = std::is_sorted(keySet.begin(), keySet.end());
                KM_ASSERT(isSorted);
            }

            static constexpr size_t maxCapacity() noexcept {
                return kLeafOrder;
            }

            std::span<Key> keys() noexcept { return std::span(mKeys, count()); }
            std::span<const Key> keys() const noexcept { return std::span(mKeys, count()); }

            std::span<Value> values() noexcept { return std::span(mValues, count()); }
            std::span<const Value> values() const noexcept { return std::span(mValues, count()); }

            Key& key(size_t index) noexcept {
                auto keySet = keys();
                return keySet[index];
            }

            const Key& key(size_t index) const noexcept {
                auto keySet = keys();
                return keySet[index];
            }

            Value& value(size_t index) noexcept {
                auto valueSet = values();
                return valueSet[index];
            }

            const Value& value(size_t index) const noexcept {
                auto valueSet = values();
                return valueSet[index];
            }

            Entry get(size_t index) const noexcept {
                KM_ASSERT(index < count());
                return { mKeys[index], mValues[index] };
            }

            void emplace(size_t index, const Entry& entry) noexcept {
                KM_ASSERT(index < count());
                mKeys[index] = entry.key;
                mValues[index] = entry.value;
            }

            InsertResult insert(const Entry& entry) noexcept {
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

            Key minKey() const noexcept {
                KM_ASSERT(!isEmpty());
                return key(0);
            }

            Key maxKey() const noexcept {
                KM_ASSERT(!isEmpty());
                return key(count() - 1);
            }
        };

        static constexpr size_t kInternalOrder = std::max(3zu, (kTargetSize - sizeof(Base)) / (sizeof(Key) + sizeof(Value) + sizeof(Base*)));

        class Internal : public Base {
            Key mKeys[kInternalOrder];
            Value mValues[kInternalOrder];
            Base *mChildren[kInternalOrder + 1];

        public:
            using Base::count;

            Internal(Internal *parent) noexcept
                : Base(false, parent)
            { }

            size_t capacity() const noexcept override {
                return maxCapacity();
            }

            static constexpr size_t maxCapacity() noexcept {
                return kInternalOrder;
            }

            std::span<Key> keys() noexcept {
                return std::span(mKeys, count());
            }

            std::span<const Key> keys() const noexcept {
                return std::span(mKeys, count());
            }

            std::span<Value> values() noexcept {
                return std::span(mValues, count());
            }

            std::span<const Value> values() const noexcept {
                return std::span(mValues, count());
            }

            std::span<Base*> children() noexcept {
                return std::span(mChildren, count() + 1);
            }

            std::span<const Base*> children() const noexcept {
                return std::span(mChildren, count() + 1);
            }
        };

        template<typename T, typename... Args>
        static T* newNode(Args&&... args) noexcept {
            return new (std::nothrow) T(std::forward<Args>(args)...);
        }

        static void destroyNode(Base *node) noexcept {
            delete node;
        }
    };
}
