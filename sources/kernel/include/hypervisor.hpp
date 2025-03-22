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

        bool invariantTsc;

        CoreMultiplier coreClock;
        mp::quantity<si::hertz, uint32_t> busClock;

        uint32_t l1ecx;
        uint32_t l1edx;

        uint32_t l7ebx;
        uint32_t l7ecx;
        uint32_t l7edx;

        bool tsc() const { return l1edx & (1 << 4); }
        bool lapic() const { return l1edx & (1 << 9); }
        bool fxsave() const { return l1edx & (1 << 24); }
        bool sse() const { return l1edx & (1 << 25); }
        bool sse2() const { return l1edx & (1 << 26); }

        bool sse3() const { return l1ecx & (1 << 0); }
        bool ssse3() const { return l1ecx & (1 << 9); }
        bool sse4_1() const { return l1ecx & (1 << 19); }
        bool sse4_2() const { return l1ecx & (1 << 20); }
        bool x2apic() const { return l1ecx & (1 << 21); }
        bool movbe() const { return l1ecx & (1 << 22); }
        bool popcnt() const { return l1ecx & (1 << 23); }
        bool tscDeadline() const { return l1ecx & (1 << 24); }
        bool aesni() const { return l1ecx & (1 << 25); }
        bool xsave() const { return l1ecx & (1 << 26); }
        bool osxsave() const { return l1ecx & (1 << 27); }
        bool avx() const { return l1ecx & (1 << 28); }
        bool f16c() const { return l1ecx & (1 << 29); }
        bool rdrand() const { return l1ecx & (1 << 30); }
        bool hypervisor() const { return l1ecx & (1 << 31); }

        bool avx2() const { return l7ebx & (1 << 5); }
        bool avx512f() const { return l7ebx & (1 << 16); }
        bool la57() const { return l7ecx & (1 << 16); }
        bool umip() const { return l7ecx & (1 << 2); }

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
