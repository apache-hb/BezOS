#pragma once

#include "arch/generic/mmu.hpp"

namespace arch {
    namespace sparcv9::detail {
        ///
        /// SPARC64 XII Specification - 13.8. Page Sizes
        static constexpr size_t kMaxPageSizeCount = 6;

        struct SparcMMUInfo {
            uintptr_t maxVirtualAddressBitWidth;
            uintptr_t maxPhysicalAddressBitWidth;
        };
    }

    struct MmuSparcV9 : GenericMmu {
        constexpr void enumPageSizes(uintptr_t *sizes, size_t *count) const noexcept;

        constexpr uintptr_t maxVirtualAddressBitWidth() const noexcept;

        constexpr uintptr_t maxPhysicalAddressBitWidth() const noexcept;

        static constexpr size_t GetMaxPageSizeCount() noexcept {
            // SPARC64 XII Specification - 13.8. Page Sizes
            return sparcv9::detail::kMaxPageSizeCount;
        }

    private:
        using SparcMMUInfo = sparcv9::detail::SparcMMUInfo;
    };

    using Mmu = MmuSparcV9;
}
