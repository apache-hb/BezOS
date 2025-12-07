#pragma once

#include <bezos/status.h>

#include "common/util/util.hpp"
#include "allocator/allocator.hpp"

#include <atomic>
#include <memory>

namespace sm {
    /// @brief A multi-producer, single-consumer atomic forward list.
    ///
    /// This is a lock-free data structure that allows multiple threads to
    /// push elements onto the front of the list and a single thread to pop
    /// elements off the front of the list.
    ///
    /// @tparam T The type of the elements in the list.
    /// @tparam Allocator The allocator to use for the list nodes.
    template<typename T, typename Allocator = sm::allocator<std::byte>>
    class AtomicForwardList {
        struct ListNode {
            T value;
            std::atomic<ListNode*> next;
        };

        using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<ListNode>;
    public:

        UTIL_NOCOPY(AtomicForwardList);

        /// @brief Construct an empty list.
        ///
        /// @param allocator The allocator to use for the list nodes.
        ///
        /// @details This requires external synchronization.
        constexpr AtomicForwardList(Allocator allocator = Allocator{})
            : AtomicForwardList(nullptr, allocator)
        { }

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
                    mAllocator.deallocate(head, 1);
                    return value;
                }
            }

            return otherwise;
        }

        /// @brief Add an element to the front of the list.
        ///
        /// @details This is internally synchronized.
        OsStatus push(T value) {
            void *storage = mAllocator.allocate(1);
            if (storage == nullptr) {
                return OsStatusOutOfMemory;
            }

            ListNode* head = mHead;
            ListNode* node = new (storage) ListNode{ std::move(value), head };
            while (!mHead.compare_exchange_strong(head, node)) {
                //
                // If the head has changed, we need to update the next pointer.
                //
                node->next = head;
            }

            return OsStatusSuccess;
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
            std::swap(lhs.mAllocator, rhs.mAllocator);
            std::swap(lhs.mHead, rhs.mHead);
        }

    private:
        AtomicForwardList(ListNode *head, NodeAllocator allocator = NodeAllocator{})
            : mAllocator(allocator)
            , mHead(head)
        { }

        [[no_unique_address]] NodeAllocator mAllocator;
        std::atomic<ListNode*> mHead = nullptr;

        constexpr void destroy() {
            for (ListNode *node = mHead; node != nullptr; ) {
                ListNode *next = node->next;
                mAllocator.deallocate(node, 1);
                node = next;
            }
        }
    };
}
