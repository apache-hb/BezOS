#pragma once

#include <stddef.h>
#include <stdint.h>

namespace arch {
    struct GenericMmu {
        [[gnu::error("enumPageSizes not implemented by platform")]]
        constexpr void enumPageSizes(uintptr_t *sizes, size_t *count) const noexcept;

        [[gnu::error("maxVirtualAddressBitWidth not implemented by platform")]]
        constexpr uintptr_t maxVirtualAddressBitWidth() const noexcept;

        [[gnu::error("maxPhysicalAddressBitWidth not implemented by platform")]]
        constexpr uintptr_t maxPhysicalAddressBitWidth() const noexcept;

        [[gnu::error("GetMaxPageSizeCount not implemented by platform")]]
        static constexpr size_t GetMaxPageSizeCount() noexcept;
    };
}
