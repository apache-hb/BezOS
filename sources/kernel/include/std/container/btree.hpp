#pragma once

#include "arch/paging.hpp"
#include "common/util/util.hpp"

#include "panic.hpp"

#include "allocator/allocator.hpp"
#include "std/layout.hpp"
#include <functional>
#include <limits.h>

namespace sm {
    namespace detail {
        enum class BTreeNodeType {
            /// @brief The entry is empty, no key or leaf pointer is stored.
            eEmpty = 0,
            /// @brief The entry is a leaf node pointer, pointing to another BTreeNode.
            eLeaf = 1,
            /// @brief The entry is a key
            eKey = 2,
        };

        class BTreeNodeHeader {
            static constexpr auto kLeafFlag = uint16_t(1 << ((sizeof(uint16_t) * CHAR_BIT) - 1));
            uint16_t mOrder;

        public:
            constexpr BTreeNodeHeader(uint16_t order, bool leaf = true) noexcept
                : mOrder(leaf ? (order | kLeafFlag) : order)
            { }

            constexpr uint16_t getOrder() const noexcept {
                return mOrder & ~kLeafFlag;
            }

            constexpr bool isLeaf() const noexcept {
                return mOrder & kLeafFlag;
            }

            void setLeaf(bool leaf) noexcept {
                if (leaf) {
                    mOrder |= kLeafFlag;
                } else {
                    mOrder &= kLeafFlag;
                }
            }
        };

        struct BTreeNodeLayout {
            Layout key;
            Layout value;

            constexpr size_t flagDataOffset(size_t) const noexcept {
                return 0;
            }

            constexpr size_t flagDataSize(size_t order) const noexcept {
                return order / 4;
            }

            constexpr size_t keyDataOffset(size_t order) const noexcept {
                size_t base = flagDataOffset(order) + flagDataSize(order) + sizeof(BTreeNodeHeader);
                return sm::roundup(base, key.align) - sizeof(BTreeNodeHeader);
            }

            constexpr size_t keyDataSize(size_t order) const noexcept {
                return key.size * order;
            }

            constexpr size_t valueDataOffset(size_t order) const noexcept {
                size_t base = keyDataOffset(order) + keyDataSize(order) + sizeof(BTreeNodeHeader);
                return sm::roundup(base, value.align) - sizeof(BTreeNodeHeader);
            }

            constexpr size_t valueDataSize(size_t order) const noexcept {
                return value.size * order;
            }

            template<typename Key, typename Value>
            static consteval BTreeNodeLayout of() noexcept {
                return {Layout::of<Key>(), Layout::of<Value>()};
            }
        };

        // TODO: This should be configurable depending on the architecture.
        constexpr size_t kTargetPageSize = x64::kPageSize * 2;

        constexpr size_t computeNodeSize(Layout keyLayout, Layout valueLayout, size_t order) {
            // The node consists of:
            // - Leaf flags: 1 byte per 4 keys
            // - Keys: sizeof(Key) * order
            // - Values: sizeof(Value) * order
            // - Node overhead: sizeof(BTreeNodeHeader)
            //
            // The leaf flags take up 1 byte for every 4 keys, so we need to account for that.
            size_t leafFlagsSize = sm::roundup(order, 4zu); // Round up to the nearest byte
            return sizeof(BTreeNodeHeader)
                 + leafFlagsSize
                 + sm::roundup(keyLayout.size * order, keyLayout.align)
                 + sm::roundup(valueLayout.size * order, valueLayout.align);
        }

        constexpr size_t computeMaxOrder(Layout keyLayout, Layout valueLayout, size_t targetSize) {
            // The order is the number of keys/values per node.
            // We need to ensure that the total size of the node does not exceed the target size.
            // The node consists of:
            // - Leaf flags: 1 byte per 4 keys
            // - Keys: sizeof(Key) * order
            // - Values: sizeof(Value) * order
            // - Node overhead: sizeof(BTreeNodeHeader)
            auto remainingSize = targetSize - sizeof(BTreeNodeHeader);
            auto maxEntries = (remainingSize / (((keyLayout.size + valueLayout.size) * 4) + 1)) * 4;
            auto leafFlagsSize = maxEntries;
            auto actualEntries = (remainingSize - leafFlagsSize) / (keyLayout.size + valueLayout.size);

            return actualEntries;
        }

        template<typename Key, typename Value>
        consteval size_t nodeAlignment() noexcept {
            return std::max({ alignof(Key), alignof(Value), alignof(void*) });
        }

        template<typename Key, typename Value>
        class alignas(nodeAlignment<Key, Value>()) BTreeNode {
            BTreeNodeHeader mHeader;

            // Data storage for keys, values, and leaf flags.
            // Each leaf flag is 2 bits, indicating whether the corresponding key is valid and whether the key is a child pointer.
            // The layout is as follows:
            // [0..(order/4)] - Leaf flags (1 byte per 4 keys)
            // [(order/8)..(order*sizeof(Key))] - Keys and leaf pointers (aligned to Key type)
            // [(order*sizeof(Key))..(order*sizeof(Value))] - Values (aligned to Value type)
            // If a node is a leaf, the leaf flags will indicate which keys are valid.
            char mStorage[];

        public:
            BTreeNode(uint16_t order, bool leaf = true)
                : mHeader(order, leaf)
            { }

            uint16_t getOrder() const noexcept {
                return mHeader.getOrder();
            }

            bool isLeaf() const noexcept {
                return mHeader.isLeaf();
            }

            static constexpr size_t computeNodeSize(size_t order) {
                return detail::computeNodeSize(Layout::of<Key>(), Layout::of<Value>(), order);
            }

            static BTreeNode *create(size_t order) {
                size_t nodeSize = computeNodeSize(order);
                if (char *storage = new (std::align_val_t(alignof(BTreeNode)), std::nothrow) char[nodeSize]) {
                    BTreeNode *node = new (storage) BTreeNode(order);
                    return node;
                }

                return nullptr;
            }

            static void destroy(BTreeNode *node [[gnu::nonnull]]) {
                std::destroy_at(node);
                operator delete[](reinterpret_cast<char*>(node), std::align_val_t(alignof(BTreeNode)), std::nothrow);
            }
        };
    }

    template<
        typename Key,
        typename Value,
        typename Compare = std::less<Key>,
        typename Equal = std::equal_to<Key>,
        typename Allocator = mem::GlobalAllocator<std::pair<const Key, Value>>
    >
    class BTreeMap {
        using Node = detail::BTreeNode<Key, Value>;

        [[no_unique_address]] Allocator mAllocator;

        Node *allocateNode(size_t order) {
            return Node::create(order);
        }

        Node *mRootNode;
    public:
        BTreeMap() : mRootNode(nullptr) { }

        ~BTreeMap() {
            if (mRootNode) {
                mAllocator.deallocate(mRootNode, Node::computeNodeSize(mRootNode->getOrder()));
            }
        }
    };
}
