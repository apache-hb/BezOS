#pragma once

#include "std/static_string.hpp"

#include <stdint.h>

namespace km {
    bool KmIsHypervisorPresent(void);

    struct HypervisorInfo {
        stdx::StaticString<12> name;
        uint32_t maxleaf;

        bool isKvm(void) const;
        bool isQemu(void) const;
        bool isMicrosoftHyperV(void) const;

        bool platformHasDebugPort(void) const;
    };

    /// @pre: IsHypervisorPresent() = true
    HypervisorInfo KmGetHypervisorInfo(void);

    struct CoreMultiplier {
        uint32_t tsc;
        uint32_t core;
    };

    using VendorString = stdx::StaticString<12>;
    using BrandString = stdx::StaticString<48>;

    struct ProcessorInfo {
        VendorString vendor;
        BrandString brand;
        uint32_t maxleaf;
        uintptr_t maxpaddr;
        uintptr_t maxvaddr;

        bool hasLocalApic;
        bool has2xApic;

        CoreMultiplier coreClock;
        uint32_t busClock; // in hz

        bool hasNominalFrequency() const {
            return busClock != 0;
        }

        uint16_t baseFrequency; // in mhz
        uint16_t maxFrequency; // in mhz
        uint16_t busFrequency; // in mhz
    };

    BrandString KmGetBrandString();

    ProcessorInfo KmGetProcessorInfo();
}