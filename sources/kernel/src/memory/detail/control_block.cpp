#include "memory/detail/control_block.hpp"

#include <stdint.h>

using ControlBlock = km::detail::ControlBlock;

void Partition(ControlBlock **mid, ControlBlock **lte, ControlBlock **gt, ControlBlock *l)
{
    ControlBlock *result = nullptr;
    ControlBlock *next = nullptr;
    ControlBlock *value = l;

    *mid = *gt = *lte = nullptr;

    if (l != nullptr)
    {
        *mid = l;
        l = l->next;
    }

    while (l != nullptr)
    {
        next = l->next;

        if (l > value)
        {
            if (l->prev != nullptr)
            {
                l->prev->next = l->next;
            }
            if (l->next != nullptr)
            {
                l->next->prev = l->prev;
            }

            if (result == nullptr)
            {
                l->next = nullptr;
            }
            else
            {
                l->next = result;
                result->prev = l;
            }

            l->prev = nullptr;
            result = l;
        }
        else
        {
            if (*lte == nullptr)
            {
                *lte = l;
            }
        }

        l = next;
    }

    *gt = result;
}

/// @brief Sort a list of control blocks by their address using quicksort
///
/// @author Andrew Haisley
void QuicksortInner(ControlBlock *l, ControlBlock **begin, ControlBlock **end)
{
    if (l == nullptr)
    {
        *begin = *end = nullptr;
    }
    else if (l->next == nullptr)
    {
        *begin = *end = l;
    }
    else
    {
        ControlBlock *lte = nullptr;
        ControlBlock *lte_end = nullptr;
        ControlBlock *gt = nullptr;
        ControlBlock *gt_end = nullptr;
        ControlBlock *mid = nullptr;

        Partition(&mid, &lte, &gt, l);

        QuicksortInner(lte, &lte, &lte_end);
        QuicksortInner(gt, &gt, &gt_end);

        if (gt == nullptr)
        {
            mid->next = nullptr;
            mid->prev = nullptr;
            gt = mid;
            *end = mid;
        }
        else
        {
            mid->next = gt;
            mid->prev = nullptr;
            gt->prev = mid;
            gt = mid;
            *end = gt_end;
        }

        if (lte == nullptr)
        {
            *begin = gt;
        }
        else
        {
            lte_end->next = gt;
            gt->prev = lte_end;
            *begin = lte;
        }
    }
}

ControlBlock *Quicksort(ControlBlock *head) {
    ControlBlock *begin = nullptr;
    ControlBlock *end = nullptr;

    QuicksortInner(head, &begin, &end);

    if (begin) {
        begin->prev = nullptr;
    }

    if (end) {
        end->next = nullptr;
    }

    return begin;
}

void km::detail::sortBlocks(ControlBlock *block) noexcept [[clang::nonallocating]] {
    block = block->head();

    Quicksort(block);
}

void km::detail::mergeAdjacentBlocks(ControlBlock *head) noexcept [[clang::nonallocating]] {
    ControlBlock *block = head;

    while (block != nullptr) {
        detail::ControlBlock *next = block->next;

        if (next == nullptr) {
            break;
        }

        if ((uintptr_t)block + block->size == (uintptr_t)next && (next->slide == block->slide)) {
            block->size += next->size;
            block->next = next->next;

            if (next->next != nullptr) {
                next->next->prev = block;
            }
        } else {
            block = block->next;
        }
    }
}
