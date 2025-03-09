#include "memory/detail/control_block.hpp"

#include "panic.hpp"

using ControlBlock = km::detail::ControlBlock;

static bool AreBlocksAdjacent(const km::detail::ControlBlock *a, const km::detail::ControlBlock *b) {
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
