#pragma once

#include "arch/generic/mmu.hpp"

#include "pci/vendor.hpp"

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

#if 0
        [[gnu::error("getCpuBrandString not implemented by platform")]]
        constexpr BrandString getCpuBrandString() const noexcept;

        [[gnu::error("getFpuBrandString not implemented by platform")]]
        constexpr BrandString getFpuBrandString() const noexcept;
#endif

        [[gnu::error("isHypervisorPresent not implemented by platform")]]
        constexpr bool isHypervisorPresent() const noexcept;

        [[gnu::error("getHypervisorName not implemented by platform")]]
        constexpr VendorString getHypervisorName() const noexcept;

        [[gnu::error("getVendorId not implemented by platform")]]
        constexpr pci::VendorId getVendorId() const noexcept;

        [[gnu::error("getMmu not implemented by platform")]]
        constexpr Mmu getMmu() const noexcept;
    };
}
