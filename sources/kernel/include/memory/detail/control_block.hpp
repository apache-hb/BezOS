#pragma once

#include <stddef.h>

namespace km::detail {
    struct ControlBlock {
        ControlBlock *next;
        ControlBlock *prev;
        size_t size;
    };

    void SortBlocks(ControlBlock *head);
    void MergeAdjacentBlocks(ControlBlock *head);
    void SwapAdjacentBlocks(ControlBlock *a, ControlBlock *b);
    void SwapAnyBlocks(ControlBlock *a, ControlBlock *b);
}
