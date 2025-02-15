#pragma once

#include "allocator/allocator.hpp"
#include "util/util.hpp"

#include <atomic>

namespace sm {
    template<typename T, typename Allocator = mem::GlobalAllocator<T>>
    class AtomicForwardList {
        struct ListNode {
            T mValue;
            std::atomic<ListNode*> mNext;
        };

        std::atomic<ListNode*> mHead = nullptr;

        constexpr void destroy() {
            for (ListNode *node = mHead; node != nullptr; ) {
                ListNode *next = node->mNext;
                delete node;
                node = next;
            }
        }

    public:
        UTIL_NOCOPY(AtomicForwardList);

        /// @brief Construct an empty list.
        ///
        /// @details This requires external synchronization.
        constexpr AtomicForwardList() = default;

        /// @details This requires external synchronization.
        constexpr AtomicForwardList(AtomicForwardList&& other) noexcept {
            std::swap(*this, other);
        }

        /// @details This requires external synchronization.
        constexpr AtomicForwardList& operator=(AtomicForwardList&& other) noexcept {
            std::swap(*this, other);
            return *this;
        }

        /// @details This requires external synchronization.
        constexpr ~AtomicForwardList() {
            destroy();
        }

        /// @brief Pop the head off the list and return its value.
        ///
        /// @details This is internally synchronized.
        T pop(T otherwise) {
            ListNode* head = mHead;

            while (head != nullptr) {
                //
                // Swap the head with the next node. If the head hasn't changed
                // since we read it, we can return the value.
                //
                if (mHead.compare_exchange_strong(head, head->mNext)) {
                    T value = head->mValue;
                    delete head;
                    return value;
                }
            }

            return otherwise;
        }

        /// @brief Add an element to the front of the list.
        ///
        /// @details This is internally synchronized.
        void push(T value) {
            ListNode* head = mHead;
            ListNode* node = new ListNode(std::move(value), head);
            while (!mHead.compare_exchange_strong(head, node)) {
                //
                // If the head has changed, we need to update the next pointer.
                //
                node->mNext = head;
            }
        }

        /// @details This requires external synchronization.
        constexpr friend void swap(AtomicForwardList& lhs, AtomicForwardList& rhs) {
            std::swap(lhs.mHead, rhs.mHead);
        }
    };
}
