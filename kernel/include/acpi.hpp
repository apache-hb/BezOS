#pragma once

#include "util/format.hpp"

#include "memory.hpp"

namespace km {
    class IoApic;
}

namespace acpi {
    struct Madt;

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

    struct [[gnu::packed]] RsdtHeader {
        char signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        char oemid[6];
        char tableId[8];
        uint32_t oemRevision;
        uint32_t creatorId;
        uint32_t creatorRevision;
    };

    static_assert(sizeof(RsdtHeader) == 36);

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

    enum class AddressSpaceId : uint8_t {
        eSystemMemory = 0x00,
        eSystemIo = 0x01,
        ePciConfig = 0x02,
        eEmbeddedController = 0x03,
        eSmbus = 0x04,
        eSystemCmos = 0x05,
        ePciBarTarget = 0x06,
        eIpmi = 0x07,
        eGeneralPurposeIo = 0x08,
        eGenericSerialBus = 0x09,
        ePlatformCommChannel = 0x0A,
        eFunctionalFixedHardware = 0x7F,
    };

    enum class AccessSize : uint8_t {
        eUndefined = 0,
        eByte = 1,
        eWord = 2,
        eDword = 3,
        eQword = 4,
    };

    // ACPI 5.2.3.2. Generic Address Structure
    struct [[gnu::packed]] GenericAddress {
        AddressSpaceId addressSpace;
        uint8_t width;
        uint8_t offset;
        AccessSize accessSize;
        uint64_t address;
    };

    static_assert(sizeof(GenericAddress) == 12);

    struct [[gnu::packed]] Fadt {
        RsdtHeader header; // signature must be "FACP"

        uint32_t firmwareCtrl;
        uint32_t dsdt;
        uint8_t reserved0[1];
        uint8_t preferredPmProfile;
        uint16_t sciInt;
        uint32_t smiCmd;
        uint8_t acpiEnable;
        uint8_t acpiDisable;
        uint8_t s4BiosReq;
        uint8_t pstateCnt;

        uint32_t pm1aEvtBlk;
        uint32_t pm1bEvtBlk;
        uint32_t pm1aCntBlk;
        uint32_t pm1bCntBlk;
        uint32_t pm2CntBlk;
        uint32_t pmTmrBlk;
        uint32_t gpe0Blk;
        uint32_t gpe1Blk;

        uint8_t pm1EvtLen;
        uint8_t pm1CntLen;
        uint8_t pm2CntLen;
        uint8_t pmTmrLen;
        uint8_t gpe0BlkLen;
        uint8_t gpe1BlkLen;
        uint8_t gpe1Base;
        uint8_t cstateCtl;
        uint16_t worstC2Latency;
        uint16_t worstC3Latency;
        uint16_t flushSize;
        uint16_t flushStride;
        uint8_t dutyOffset;
        uint8_t dutyWidth;
        uint8_t dayAlrm;
        uint8_t monAlrm;
        uint8_t century;
        uint16_t iapcBootArch;
        uint8_t reserved1[1];
        uint32_t flags;
        GenericAddress resetReg;
        uint8_t resetValue;
        uint16_t armBootArch;
        uint8_t fadtMinorVersion;
        uint64_t x_firmwareCtrl;
        uint64_t x_dsdt;

        GenericAddress x_pm1aEvtBlk;
        GenericAddress x_pm1bEvtBlk;
        GenericAddress x_pm1aCntBlk;
        GenericAddress x_pm1bCntBlk;
        GenericAddress x_pm2CntBlk;
        GenericAddress x_pmTmrBlk;
        GenericAddress x_gpe0Blk;
        GenericAddress x_gpe1Blk;
        GenericAddress sleepControlReg;
        GenericAddress sleepStatusReg;
        uint64_t hypervisorVendor;
    };

    static_assert(sizeof(Fadt) == 276);

    enum class MadtFlags : uint32_t {
        ePcatCompat = (1 << 0),
    };

    UTIL_BITFLAGS(MadtFlags);

    enum class MadtEntryType : uint8_t {
        eLocalApic = 0,
        eIoApic = 1,
    };

    struct [[gnu::packed]] MadtEntry {
        struct [[gnu::packed]] LocalApic {
            uint8_t processorId;
            uint8_t apicId;
            uint32_t flags;
        };

        struct [[gnu::packed]] IoApic {
            uint8_t ioApicId;
            uint8_t reserved;
            uint32_t address;
            uint32_t interruptBase;
        };

        MadtEntryType type;
        uint8_t length;

        union {
            LocalApic lapic;
            IoApic ioapic;
        };
    };

    class MadtIterator {
        const uint8_t *mCurrent;

    public:
        MadtIterator(const uint8_t *current)
            : mCurrent(current)
        { }

        MadtIterator& operator++();

        const MadtEntry *operator*();

        friend bool operator!=(const MadtIterator& lhs, const MadtIterator& rhs);
    };

    struct Madt {
        RsdtHeader header; // signature must be "APIC"

        uint32_t localApicAddress;
        MadtFlags flags;

        // these entries are variable length so i cant use a struct
        uint8_t entries[];

        MadtIterator begin() const {
            return MadtIterator { entries };
        }

        MadtIterator end() const {
            return MadtIterator { reinterpret_cast<const uint8_t*>(this) + header.length };
        }
    };

    class AcpiTables {
        const RsdpLocator *mRsdpLocator;
        const Madt *mMadt;

    public:
        AcpiTables(const RsdpLocator *locator, km::SystemMemory& memory);

        uint32_t revision() const { return mRsdpLocator->revision; }

        km::IoApic mapIoApic(km::SystemMemory& memory, uint32_t index) const;
        uint32_t ioApicCount() const;
    };
}

acpi::AcpiTables KmInitAcpi(km::PhysicalAddress rsdpBaseAddress, km::SystemMemory& memory);

template<>
struct km::StaticFormat<acpi::MadtEntryType> {
    static constexpr size_t kStringSize = km::StaticFormat<uint8_t>::kStringSize;
    static stdx::StringView toString(char *buffer, acpi::MadtEntryType type) {
        switch (type) {
        case acpi::MadtEntryType::eLocalApic:
            return "Local APIC";
        case acpi::MadtEntryType::eIoApic:
            return "IO APIC";
        default:
            return km::StaticFormat<uint8_t>::toString(buffer, static_cast<uint8_t>(type));
        }
    }
};
