#pragma once

#include "memory/range.hpp"
#include <stddef.h>
#include <stdint.h>

namespace km::detail {
    struct ControlBlock {
        ControlBlock *next;
        ControlBlock *prev;
        size_t size;
        uintptr_t slide;

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

        km::VirtualRangeEx range() const noexcept [[clang::nonblocking]] {
            return { (uintptr_t)this, (uintptr_t)this + size };
        }
    };

    void sortBlocks(ControlBlock *head) noexcept [[clang::nonallocating]] ;
    void mergeAdjacentBlocks(ControlBlock *head) noexcept [[clang::nonallocating]] ;
}
