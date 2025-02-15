#pragma once

#include "allocator/allocator.hpp"
#include "util/util.hpp"

#include <atomic>

namespace sm {
    template<typename T, typename Allocator = mem::GlobalAllocator<T>>
    class AtomicForwardList {
        struct ListNode {
            T value;
            std::atomic<ListNode*> next;
        };
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
                if (mHead.compare_exchange_strong(head, head->next)) {
                    T value = head->value;
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
            ListNode* node = new ListNode{ .value = std::move(value), .next = head };
            while (!mHead.compare_exchange_strong(head, node)) {
                //
                // If the head has changed, we need to update the next pointer.
                //
                node->next = head;
            }
        }

        AtomicForwardList exchange(AtomicForwardList&& replace) {
            //
            // First we need to acquire the head node of the new list to ensure it doesnt
            // change while we're doing the rest of the work.
            //
            ListNode *other = replace.mHead.exchange(nullptr);

            //
            // Now we can swap the head pointer of the other list, which we now own
            // with the head pointer of ourselves.
            //
            ListNode *head = mHead.exchange(other);

            //
            // And now we return a newly created list with our old head pointer.
            //
            return AtomicForwardList(head);
        }

        /// @details This requires external synchronization.
        constexpr friend void swap(AtomicForwardList& lhs, AtomicForwardList& rhs) {
            std::swap(lhs.mHead, rhs.mHead);
        }

    private:
        AtomicForwardList(ListNode *head)
            : mHead(head)
        { }

        std::atomic<ListNode*> mHead = nullptr;

        constexpr void destroy() {
            for (ListNode *node = mHead; node != nullptr; ) {
                ListNode *next = node->next;
                delete node;
                node = next;
            }
        }
    };
}
