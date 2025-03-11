#pragma once

#include <stddef.h>

namespace km::detail {
    struct ControlBlock {
        ControlBlock *next;
        ControlBlock *prev;
        size_t size;

        ControlBlock *head() noexcept {
            ControlBlock *result = this;
            while (result->prev != nullptr) {
                result = result->prev;
            }
            return result;
        }

        const ControlBlock *head() const noexcept {
            const ControlBlock *result = this;
            while (result->prev != nullptr) {
                result = result->prev;
            }
            return result;
        }
    };

    void SortBlocks(ControlBlock *head);
    void MergeAdjacentBlocks(ControlBlock *head);
}
