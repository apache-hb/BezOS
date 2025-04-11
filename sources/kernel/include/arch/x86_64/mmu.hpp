#pragma once

#include "util/memory.hpp"

#include "arch/generic/mmu.hpp"

namespace arch {
    struct MmuX86_64 : GenericMmu {
        constexpr void enumPageSizes(uintptr_t *sizes, size_t *count) const noexcept {
            sizes[0] = sm::kilobytes(4).bytes();
            sizes[1] = sm::megabytes(2).bytes();
            sizes[2] = sm::gigabytes(1).bytes();
            *count = 3;
        }

        constexpr uintptr_t maxVirtualAddressBitWidth() const noexcept;
        constexpr uintptr_t maxPhysicalAddressBitWidth() const noexcept;

        static constexpr size_t GetMaxPageSizeCount() noexcept {
            return 3;
        }
    };

    using Mmu = MmuX86_64;
}
