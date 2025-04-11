#pragma once

#include "arch/generic/machine.hpp"

#include "std/static_string.hpp"

namespace arch {
    namespace sparcv9::detail {
        using BrandString = stdx::StaticString<48>;
        using VendorString = stdx::StaticString<32>;

        struct SparcV9MachineInfo {
            VendorString vendor;
            BrandString cpuBrand;
            BrandString fpuBrand;
            BrandString pmuBrand;
        };

        [[gnu::always_inline, gnu::nodebug]]
        static inline uint32_t rdver() noexcept {
            uint32_t ver;
            asm volatile("rdpr %%ver, %0" : "=r"(ver));
            return ver;
        }

        enum ChipId : uint8_t {
            eChipInvalid = 0x00,
            eChipUltra1 = 0x01,
            eChipUltra2 = 0x02,
            eChipUltra3 = 0x03,
            eChipUltra4 = 0x04,
            eChipUltra5 = 0x05,
            eChipSparcM6 = 0x06,
            eChipSparcM7 = 0x07,
            eChipSparcM8 = 0x08,
        };

        SparcV9MachineInfo GetMachineInfo(uint32_t ver) noexcept;
    }

    struct MachineSparcV9 : GenericMachine {
        using VendorString = sparcv9::detail::VendorString;
        using BrandString = sparcv9::detail::BrandString;

        static MachineSparcV9 GetInfo() noexcept {
            return MachineSparcV9(sparcv9::detail::GetMachineInfo(sparcv9::detail::rdver()));
        }

        constexpr VendorString getVendorString() const noexcept {
            return mInfo.vendor;
        }

        constexpr BrandString getCpuBrandString() const noexcept {
            return mInfo.cpuBrand;
        }

        constexpr BrandString getFpuBrandString() const noexcept {
            return mInfo.fpuBrand;
        }

        constexpr BrandString getBrandString() const noexcept {
            return mInfo.pmuBrand;
        }

    private:
        MachineSparcV9(sparcv9::detail::SparcV9MachineInfo info) noexcept
            : GenericMachine()
            , mInfo(info)
        { }

        sparcv9::detail::SparcV9MachineInfo mInfo;
    };

    using Machine = MachineSparcV9;
}
