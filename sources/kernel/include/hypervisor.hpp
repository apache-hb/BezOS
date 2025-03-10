#pragma once

#include "std/static_string.hpp"

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

    enum class XSaveFeature {
        FP = 0,
        SSE = 1,

        YMM = 2,
        BNDREGS = 3,
        BNDCSR = 4,
        OPMASK = 5,
        ZMM_HI256 = 6,
        HI16_ZMM = 7,
        PKRU = 9,
        PASID = 10,
        CET = 11,
        LBR = 15,
        XTILECFG = 17,
        XTILEDATA = 18,
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
        uint32_t busClock; // in hz

        bool xsave;
        bool xsaveopt;
        bool xsavec;
        bool xsaves;
        bool xgetbvext;

        bool hasNominalFrequency() const {
            return busClock != 0;
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

        uint16_t baseFrequency; // in mhz
        uint16_t maxFrequency; // in mhz
        uint16_t busFrequency; // in mhz
    };

    BrandString GetBrandString();

    ProcessorInfo GetProcessorInfo();
}
