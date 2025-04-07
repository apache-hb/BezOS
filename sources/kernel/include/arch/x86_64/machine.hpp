#pragma once

#include "arch/generic/machine.hpp"
#include "std/static_string.hpp"

namespace arch {
    struct MachineX86_64 : GenericMachine {
        using BrandString = stdx::StaticString<48>;
        using VendorString = stdx::StaticString<12>;

        [[gnu::always_inline, gnu::nodebug]]
        constexpr BrandString getBrandString() const noexcept {
            return BrandString();
        }

        [[gnu::always_inline, gnu::nodebug]]
        constexpr VendorString getVendorString() const noexcept {
            return VendorString();
        }
    };

    using Machine = MachineX86_64;
}
