#include "memory/detail/control_block.hpp"

#include "panic.hpp"

#define KM_ENABLE_QUICKSORT 1

using ControlBlock = km::detail::ControlBlock;

static bool AreBlocksAdjacent(const ControlBlock *a, const ControlBlock *b) {
    if (a->next == b && b->prev == a) {
        return true;
    }

    if (b->next == a && a->prev == b) {
        return true;
    }

    return false;
}

void km::detail::SwapAdjacentBlocks(ControlBlock *a, ControlBlock *b) {
    KM_CHECK(a->next == b, "Blocks are not adjacent.");
    KM_CHECK(b->prev == a, "Blocks are not adjacent.");

    ControlBlock tmpa = *a;
    ControlBlock tmpb = *b;

    if (tmpa.prev) {
        tmpa.prev->next = b;
    }

    if (tmpb.next) {
        tmpb.next->prev = a;
    }

    a->next = tmpb.next;
    a->prev = b;

    b->next = a;
    b->prev = tmpa.prev;
}

void km::detail::SwapAnyBlocks(ControlBlock *a, ControlBlock *b) {
    if (AreBlocksAdjacent(a, b)) {
        SwapAdjacentBlocks(a, b);
    } else {
        ControlBlock tmpa = *a;
        ControlBlock tmpb = *b;

        if (tmpa.prev) {
            tmpa.prev->next = b;
        }
        b->prev = tmpa.prev;

        if (tmpa.next) {
            tmpa.next->prev = b;
        }
        b->next = tmpa.next;

        if (tmpb.prev) {
            tmpb.prev->next = a;
        }
        a->prev = tmpb.prev;

        if (tmpb.next) {
            tmpb.next->prev = a;
        }
        a->next = tmpb.next;
    }
}

#if KM_ENABLE_QUICKSORT

void partition(ControlBlock **mid, ControlBlock **lte, ControlBlock **gt, ControlBlock *l)
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
void quicksort_inner(ControlBlock *l, ControlBlock **begin, ControlBlock **end)
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

        partition(&mid, &lte, &gt, l);

        quicksort_inner(lte, &lte, &lte_end);
        quicksort_inner(gt, &gt, &gt_end);

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

ControlBlock *quicksort(ControlBlock *head) {
    ControlBlock *begin = nullptr;
    ControlBlock *end = nullptr;

    quicksort_inner(head, &begin, &end);

    if (begin) {
        begin->prev = nullptr;
    }

    if (end) {
        end->next = nullptr;
    }

    return begin;
}

void km::detail::SortBlocks(ControlBlock *block) {
    block = block->head();

    quicksort(block);
}

#else

void km::detail::SortBlocks(ControlBlock *block) {
    ControlBlock *front = block->head();

    for (ControlBlock *it = front; it != nullptr; it = it->next) {
        for (ControlBlock *inner = it; inner != nullptr; inner = inner->next) {
            if (it > inner) {
                SwapAnyBlocks(it, inner);
                it = inner;
            }
        }
    }
}

#endif

void km::detail::MergeAdjacentBlocks(ControlBlock *head) {
    ControlBlock *block = head;

    while (block != nullptr) {
        detail::ControlBlock *next = block->next;

        if (next == nullptr) {
            break;
        }

        if ((uintptr_t)block + block->size == (uintptr_t)next) {
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
