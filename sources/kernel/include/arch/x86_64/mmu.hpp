#pragma once

#include "util/memory.hpp"

#include "arch/generic/mmu.hpp"

namespace arch {
    struct MmuInfoX86_64 {
        constexpr void enumPageSizes(uintptr_t *sizes, size_t *count) const noexcept {
            sizes[0] = sm::kilobytes(4).bytes();
            sizes[1] = sm::megabytes(2).bytes();
            sizes[2] = sm::gigabytes(1).bytes();
            *count = 3;
        }

        constexpr uintptr_t maxVirtualAddressBitWidth() const noexcept;
        constexpr uintptr_t maxPhysicalAddressBitWidth() const noexcept;
    };

    struct MmuX86_64 : GenericMmu {
        using MmuInfo = MmuInfoX86_64;

        static MmuInfo GetInfo() noexcept {
            return MmuInfoX86_64();
        }

        static constexpr size_t GetMaxPageSizeCount() noexcept {
            return 3;
        }
    };

    using MmuInfo = MmuInfoX86_64;
    using Mmu = MmuX86_64;
}
