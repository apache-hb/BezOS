#pragma once

#include "util/memory.hpp"

#include "arch/generic/mmu.hpp"

namespace arch {
    class MmuX86_64 : GenericMmu {
        uintptr_t mVaddrBitWidth;
        uintptr_t mPaddrBitWidth;
    public:
        MmuX86_64(uintptr_t vaddrBitWidth, uintptr_t paddrBitWidth) noexcept
            : mVaddrBitWidth(vaddrBitWidth)
            , mPaddrBitWidth(paddrBitWidth)
        { }

        // GenericMmu - begin

        constexpr void enumPageSizes(uintptr_t *sizes, size_t *count) const noexcept {
            sizes[0] = sm::kilobytes(4).bytes();
            sizes[1] = sm::megabytes(2).bytes();
            sizes[2] = sm::gigabytes(1).bytes();
            *count = 3;
        }

        constexpr uintptr_t maxVirtualAddressBitWidth() const noexcept {
            return mVaddrBitWidth;
        }

        constexpr uintptr_t maxPhysicalAddressBitWidth() const noexcept {
            return mPaddrBitWidth;
        }

        static constexpr size_t GetMaxPageSizeCount() noexcept {
            return 3;
        }

        // GenericMmu - end

        // MmuX86_64 - begin

        // MmuX86_64 - end
    };

    using Mmu = MmuX86_64;
}
