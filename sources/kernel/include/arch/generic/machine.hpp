#pragma once

namespace arch {
    struct GenericMachine {
        using BrandString = void;
        using VendorString = void;

        [[gnu::error("GetInfo not implemented by platform")]]
        static GenericMachine GetInfo() noexcept;

        [[gnu::error("getBrandString not implemented by platform")]]
        constexpr BrandString getBrandString() const noexcept;

        [[gnu::error("getVendorString not implemented by platform")]]
        constexpr VendorString getVendorString() const noexcept;

        [[gnu::error("isHypervisorPresent not implemented by platform")]]
        constexpr bool isHypervisorPresent() const noexcept;
    };
}
