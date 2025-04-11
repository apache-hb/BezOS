#pragma once

#include "arch/generic/mmu.hpp"

namespace arch {
    struct GenericMachine {
        using VendorString = void;
        using BrandString = void;
        using Mmu = GenericMmu;

        [[gnu::error("GetInfo not implemented by platform")]]
        static GenericMachine GetInfo() noexcept;

        [[gnu::error("getVendorString not implemented by platform")]]
        constexpr VendorString getVendorString() const noexcept;

        [[gnu::error("getBrandString not implemented by platform")]]
        constexpr BrandString getBrandString() const noexcept;

        [[gnu::error("getCpuBrandString not implemented by platform")]]
        constexpr BrandString getCpuBrandString() const noexcept;

        [[gnu::error("getFpuBrandString not implemented by platform")]]
        constexpr BrandString getFpuBrandString() const noexcept;

        [[gnu::error("isVirtualMachine not implemented by platform")]]
        constexpr bool isVirtualMachine() const noexcept;

        [[gnu::error("getMmu not implemented by platform")]]
        constexpr Mmu getMmu() const noexcept;
    };
}
