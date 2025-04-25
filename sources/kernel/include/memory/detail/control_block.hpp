#pragma once

#include <stddef.h>

namespace km::detail {
    struct ControlBlock {
        ControlBlock *next;
        ControlBlock *prev;
        size_t size;

        ControlBlock *head() noexcept [[clang::nonallocating]] {
            ControlBlock *result = this;
            while (result->prev != nullptr) {
                result = result->prev;
            }
            return result;
        }

        const ControlBlock *head() const noexcept [[clang::nonallocating]] {
            const ControlBlock *result = this;
            while (result->prev != nullptr) {
                result = result->prev;
            }
            return result;
        }
    };

    void SortBlocks(ControlBlock *head) noexcept [[clang::nonallocating]] ;
    void MergeAdjacentBlocks(ControlBlock *head) noexcept [[clang::nonallocating]] ;
}
