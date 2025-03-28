#pragma once

#include "memory.hpp"

#include "acpi/madt.hpp"
#include "acpi/fadt.hpp"
#include "acpi/mcfg.hpp"
#include "util/signature.hpp"

namespace km {
    class IoApic;
    class AddressSpace;
}

namespace acpi {
    struct [[gnu::packed]] RsdpLocator {
        // v1 fields
        std::array<char, 8> signature;
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
        static constexpr TableSignature kSignature = util::Signature("RSDT");

        RsdtHeader header; // signature must be "RSDT"
        uint32_t entries[];

        uint32_t count() const noexcept {
            return (header.length - sizeof(header)) / sizeof(uint32_t);
        }
    };

    struct [[gnu::packed]] Xsdt {
        static constexpr TableSignature kSignature = util::Signature("XSDT");

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
        std::unique_ptr<const RsdtHeader*[]> mRsdtEntries;
        size_t mRsdtEntryCount;

    public:
        AcpiTables(const RsdpLocator *locator, km::AddressSpace& memory);

        const RsdpLocator *locator() const { return mRsdpLocator; }
        uint32_t revision() const { return mRsdpLocator->revision; }

        uint32_t lapicCount() const;

        km::IoApic mapIoApic(km::SystemMemory& memory, uint32_t index) const;
        uint32_t ioApicCount() const;

        uint32_t hpetCount() const;

        std::span<const RsdtHeader*> entries() const {
            return std::span(mRsdtEntries.get(), mRsdtEntryCount);
        }

        bool has8042Controller() const;

        const Madt *madt() const { return mMadt; }
        const Mcfg *mcfg() const { return mMcfg; }
        const Fadt *fadt() const { return mFadt; }
        const RsdtHeader *dsdt() const { return mDsdt; }
    };

    acpi::AcpiTables InitAcpi(km::PhysicalAddress rsdpBaseAddress, km::AddressSpace& memory);
}
