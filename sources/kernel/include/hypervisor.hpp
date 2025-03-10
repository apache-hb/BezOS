#pragma once

#include "std/static_string.hpp"

#include "units.hpp"

#include <stdint.h>

namespace km {
    bool IsHypervisorPresent();

    struct HypervisorInfo {
        stdx::StaticString<12> name;
        uint32_t maxleaf;

        bool isPresent() const { return maxleaf == 0; }
        bool isKvm() const;
        bool isQemu() const;
        bool isMicrosoftHyperV() const;

        bool platformHasDebugPort() const;
    };

    std::optional<HypervisorInfo> GetHypervisorInfo();

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

        /// @brief The bits of the maximum virtual address.
        uintptr_t maxvaddr;

        bool hasLocalApic;
        bool has2xApic;
        bool tscDeadline;
        bool invariantTsc;

        CoreMultiplier coreClock;
        mp::quantity<si::hertz, uint32_t> busClock; // in hz

        bool sse;
        bool sse2;
        bool sse3;
        bool ssse3;
        bool sse4_1;
        bool sse4_2;
        bool popcnt;
        bool xsave;

        bool hasNominalFrequency() const {
            return busClock != (0 * si::hertz);
        }

        /// @brief The canonical maximum virtual address.
        uintptr_t maxVirtualAddress() const {
            return (UINTPTR_MAX << maxvaddr);
        }

        /// @brief The canonical maximum physical address.
        uintptr_t maxPhysicalAddress() const {
            return (UINTPTR_MAX << maxpaddr);
        }

        bool isKvm() const;

        bool hasBusFrequency() const {
            return maxleaf >= 0x16;
        }

        mp::quantity<si::megahertz, uint16_t> baseFrequency;
        mp::quantity<si::megahertz, uint16_t> maxFrequency;
        mp::quantity<si::megahertz, uint16_t> busFrequency;
    };

    BrandString GetBrandString();

    ProcessorInfo GetProcessorInfo();
}
