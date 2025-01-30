#pragma once

#include "memory.hpp"

#include "acpi/madt.hpp"
#include "acpi/fadt.hpp"
#include "acpi/mcfg.hpp"

namespace km {
    class IoApic;
}

namespace acpi {
    struct [[gnu::packed]] RsdpLocator {
        // v1 fields
        char signature[8];
        uint8_t checksum;
        char oemid[6];
        uint8_t revision;
        uint32_t rsdtAddress;

        // v2 fields
        uint32_t length;
        uint64_t xsdtAddress;
        uint8_t extendedChecksum;
        uint8_t reserved[3];
    };

    static_assert(sizeof(RsdpLocator) == 36);

    struct [[gnu::packed]] Rsdt {
        RsdtHeader header; // signature must be "RSDT"
        uint32_t entries[];

        uint32_t count() const noexcept {
            return (header.length - sizeof(header)) / sizeof(uint32_t);
        }
    };

    struct [[gnu::packed]] Xsdt {
        RsdtHeader header; // signature must be "XSDT"
        uint64_t entries[];

        uint32_t count() const noexcept {
            return (header.length - sizeof(header)) / sizeof(uint64_t);
        }
    };

    class AcpiTables {
        const RsdpLocator *mRsdpLocator;
        const Madt *mMadt;
        const Mcfg *mMcfg;
        const Fadt *mFadt;
        const RsdtHeader *mDsdt;

    public:
        AcpiTables(const RsdpLocator *locator, km::SystemMemory& memory);

        uint32_t revision() const { return mRsdpLocator->revision; }

        uint32_t lapicCount() const;

        km::IoApic mapIoApic(km::SystemMemory& memory, uint32_t index) const;
        uint32_t ioApicCount() const;

        uint32_t hpetCount() const;

        bool has8042Controller() const;

        const Madt *madt() const { return mMadt; }
        const Mcfg *mcfg() const { return mMcfg; }
        const Fadt *fadt() const { return mFadt; }
        const RsdtHeader *dsdt() const { return mDsdt; }
    };

    acpi::AcpiTables InitAcpi(km::PhysicalAddress rsdpBaseAddress, km::SystemMemory& memory);
}
