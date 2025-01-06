#include "acpi/acpi.hpp"
#include "apic.hpp"
#include "kernel.hpp"

#include <stddef.h>

using namespace stdx::literals;

static bool ValidateChecksum(const uint8_t *bytes, size_t length) {
    uint32_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }

    return (sum & 0xFF) == 0;
}

static bool ValidateRsdpLocator(const acpi::RsdpLocator *rsdp) {
    switch (rsdp->revision) {
    case 0:
        return ValidateChecksum((const uint8_t*)rsdp, 20);
    default:
        KmDebugMessage("[ACPI] Unknown RSDP revision: ", rsdp->revision, ". Interpreting as an XSDT.\n");
    case 2:
        return ValidateChecksum((const uint8_t*)rsdp, rsdp->length);
    }
}

static const acpi::RsdtHeader *MapTableEntry(km::PhysicalAddress paddr, km::SystemMemory& memory) {
    // first map the header
    const acpi::RsdtHeader *header = memory.hhdmMapConst<acpi::RsdtHeader>(paddr);

    // then use the headers length to ensure we map the entire table
    return memory.hhdmMapConst<acpi::RsdtHeader>(paddr, paddr + header->length);
}

static void DebugMadt(const acpi::RsdtHeader *header) {
    const acpi::Madt *madt = reinterpret_cast<const acpi::Madt*>(header);

    KmDebugMessage("| /SYS/ACPI/APIC     | Local APIC address          | ", km::Hex(madt->localApicAddress).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/APIC     | Flags                       | ", bool(madt->flags & acpi::MadtFlags::ePcatCompat) ? stdx::StringView("PCAT compatible") : stdx::StringView("None"), "\n");

    uint32_t index = 0;
    for (const acpi::MadtEntry *entry : *madt) {
        KmDebugMessage("| /SYS/ACPI/APIC/", km::rpad(3) + index, " | Entry type                  | ", entry->type, "\n");
        KmDebugMessage("| /SYS/ACPI/APIC/", km::rpad(3) + index, " | Entry length                | ", entry->length, "\n");

        index += 1;
    }
}

static void DebugMcfg(const acpi::RsdtHeader *header) {
    const acpi::Mcfg *mcfg = reinterpret_cast<const acpi::Mcfg*>(header);

    for (size_t i = 0; i < mcfg->allocationCount(); i++) {
        const acpi::McfgAllocation *allocation = &mcfg->allocations[i];
        KmDebugMessage("| /SYS/ACPI/MCFG/", km::rpad(3) + i, " | Address                     | ", km::PhysicalAddress(allocation->address), "\n");
        KmDebugMessage("| /SYS/ACPI/MCFG/", km::rpad(3) + i, " | PCI Segment                 | ", km::Hex(allocation->segment).pad(4, '0'), "\n");
        KmDebugMessage("| /SYS/ACPI/MCFG/", km::rpad(3) + i, " | Bus range                   | ",
            km::Hex(allocation->startBusNumber).pad(2, '0'), "..", km::Hex(allocation->endBusNumber).pad(2, '0'), "\n");
    }
}

static void DebugFadt(const acpi::RsdtHeader *header) {
    const acpi::Fadt *fadt = reinterpret_cast<const acpi::Fadt*>(header);

    KmDebugMessage("| /SYS/ACPI/FACP     | Firmware control            | ", km::Hex(fadt->firmwareCtrl).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | DSDT                        | ", km::Hex(fadt->dsdt).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Preferred PM profile        | ", fadt->preferredPmProfile, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | SCI interrupt               | ", fadt->sciInt, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | SMI command                 | ", km::Hex(fadt->smiCmd).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | ACPI enable                 | ", fadt->acpiEnable, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | ACPI disable                | ", fadt->acpiDisable, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | S4 BIOS request             | ", fadt->s4BiosReq, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | P-state control             | ", fadt->pstateCnt, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1A event block            | ", km::Hex(fadt->pm1aEvtBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1B event block            | ", km::Hex(fadt->pm1bEvtBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1A control block          | ", km::Hex(fadt->pm1aCntBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1B control block          | ", km::Hex(fadt->pm1bCntBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM2 control block           | ", km::Hex(fadt->pm2CntBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM timer block              | ", km::Hex(fadt->pmTmrBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE0 block                  | ", km::Hex(fadt->gpe0Blk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE1 block                  | ", km::Hex(fadt->gpe1Blk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1 event length            | ", fadt->pm1EvtLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1 control length          | ", fadt->pm1CntLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM2 control length          | ", fadt->pm2CntLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM timer length             | ", fadt->pmTmrLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE0 block length           | ", fadt->gpe0BlkLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE1 block length           | ", fadt->gpe1BlkLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE1 base                   | ", fadt->gpe1Base, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | C-state control             | ", fadt->cstateCtl, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Worst C2 latency            | ", fadt->worstC2Latency, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Worst C3 latency            | ", fadt->worstC3Latency, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Flush size                  | ", fadt->flushSize, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Flush stride                | ", fadt->flushStride, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Duty offset                 | ", fadt->dutyOffset, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Duty width                  | ", fadt->dutyWidth, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Day alarm                   | ", fadt->dayAlrm, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Month alarm                 | ", fadt->monAlrm, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Century                     | ", fadt->century, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | IAPC boot arch              | ", fadt->iapcBootArch, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Flags                       | ", km::Hex(fadt->flags).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Reset register              | ", fadt->resetReg, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Reset value                 | ", km::Hex(fadt->resetValue).pad(2, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | ARM boot arch               | ", km::Hex(fadt->armBootArch).pad(4, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | FADT minor version          | ", fadt->fadtMinorVersion, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended firmware control   | ", km::Hex(fadt->x_firmwareCtrl).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended DSDT               | ", km::Hex(fadt->x_dsdt).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1A event block   | ", fadt->x_pm1aEvtBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1B event block   | ", fadt->x_pm1bEvtBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1A control block | ", fadt->x_pm1aCntBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1B control block | ", fadt->x_pm1bCntBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM2 control block  | ", fadt->x_pm2CntBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM timer block     | ", fadt->x_pmTmrBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended GPE0 block         | ", fadt->x_gpe0Blk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended GPE1 block         | ", fadt->x_gpe1Blk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Sleep control register      | ", fadt->sleepControlReg, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Sleep status register       | ", fadt->sleepStatusReg, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Hypervisor vendor ID        | ", fadt->hypervisorVendor, "\n");
}

static const acpi::RsdtHeader *GetRsdtHeader(km::PhysicalAddress paddr, km::SystemMemory& memory) {
    const acpi::RsdtHeader *entry = MapTableEntry(paddr, memory);
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Address                     | ", paddr, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Signature                   | '", stdx::StringView(entry->signature), "'\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Length                      | ", entry->length, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Revision                    | ", entry->revision, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | OEM                         | ", stdx::StringView(entry->oemid), "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Table ID                    | ", stdx::StringView(entry->tableId), "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | OEM revision                | ", entry->oemRevision, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Creator ID                  | ", km::Hex(entry->creatorId), "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Creator revision            | ", entry->creatorRevision, "\n");

    if ("APIC"_sv == entry->signature) {
        DebugMadt(entry);
    } else if ("MCFG"_sv == entry->signature) {
        DebugMcfg(entry);
    } else if ("FACP"_sv == entry->signature) {
        DebugFadt(entry);
    }

    return entry;
}

static void DebugRsdt(const acpi::RsdpLocator *locator, km::SystemMemory& memory) {
    KmDebugMessage("| /SYS/ACPI          | RSDT address                | ", km::Hex(locator->rsdtAddress).pad(8, '0'), "\n");

    const acpi::Rsdt *rsdt = memory.hhdmMapConst<acpi::Rsdt>(locator->rsdtAddress);

    KmDebugMessage("| /SYS/ACPI/RSDT     | Signature                   | '", stdx::StringView(rsdt->header.signature), "'\n");

    for (uint32_t i = 0; i < rsdt->count(); i++) {
        km::PhysicalAddress paddr = km::PhysicalAddress { rsdt->entries[i] };
        [[maybe_unused]]
        const acpi::RsdtHeader *entry = GetRsdtHeader(paddr, memory);
    }
}

static void DebugXsdt(const acpi::RsdpLocator *locator, km::SystemMemory& memory) {
    KmDebugMessage("| /SYS/ACPI          | RSDP length                | ", locator->length, "\n");
    KmDebugMessage("| /SYS/ACPI          | XSDT address               | ", km::Hex(locator->xsdtAddress).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI          | Extended checksum          | ", locator->extendedChecksum, "\n");

    const acpi::Xsdt *xsdt = memory.hhdmMapConst<acpi::Xsdt>(km::PhysicalAddress { locator->xsdtAddress });

    KmDebugMessage("| /SYS/ACPI/XSDT     | Signature                  | '", stdx::StringView(xsdt->header.signature), "'\n");

    for (uint32_t i = 0; i < xsdt->count(); i++) {
        km::PhysicalAddress paddr = km::PhysicalAddress { xsdt->entries[i] };
        [[maybe_unused]]
        const acpi::RsdtHeader *entry = GetRsdtHeader(paddr, memory);
    }
}

acpi::AcpiTables InitAcpi(km::PhysicalAddress rsdpBaseAddress, km::SystemMemory& memory) {
    // map the rsdp table
    const acpi::RsdpLocator *locator = memory.hhdmMapConst<acpi::RsdpLocator>(rsdpBaseAddress);

    // validate that the table is ok to use
    bool rsdpOk = ValidateRsdpLocator(locator);
    KM_CHECK(rsdpOk, "Invalid RSDP checksum.");

    KmDebugMessage("| /SYS/ACPI          | RSDP signature              | '", stdx::StringView(locator->signature), "'\n");
    KmDebugMessage("| /SYS/ACPI          | RSDP checksum               | ", rsdpOk ? stdx::StringView("Valid") : stdx::StringView("Invalid"), "\n");
    KmDebugMessage("| /SYS/ACPI          | RSDP revision               | ", locator->revision, "\n");
    KmDebugMessage("| /SYS/ACPI          | OEM                         | ", stdx::StringView(locator->oemid), "\n");

    if (locator->revision == 0) {
        DebugRsdt(locator, memory);
    } else {
        DebugXsdt(locator, memory);
    }

    return acpi::AcpiTables(locator, memory);
}

acpi::MadtIterator& acpi::MadtIterator::operator++() {
    uint8_t length = *(mCurrent + 1);
    mCurrent += length;
    return *this;
}

const acpi::MadtEntry *acpi::MadtIterator::operator*() {
    return reinterpret_cast<const MadtEntry*>(mCurrent);
}

bool acpi::operator!=(const MadtIterator& lhs, const MadtIterator& rhs) {
    // prevent overrunning the end of the table in the case of a corrupted entry length
    return lhs.mCurrent < rhs.mCurrent;
}

template<typename T>
void SetUniqueTableEntry(const T** dst, const acpi::RsdtHeader *header, stdx::StringView signature) {
    if (header->signature != signature) return;
    if (*dst != nullptr) {
        KmDebugMessage("[ACPI] Multiple ", signature, " tables found. This table should be unique. I will only use the first instance of this table.\n");
        return;
    }

    *dst = reinterpret_cast<const T*>(header);
}

acpi::AcpiTables::AcpiTables(const RsdpLocator *locator, km::SystemMemory& memory)
    : mRsdpLocator(locator)
    , mMadt(nullptr)
    , mMcfg(nullptr)
    , mFadt(nullptr)
{
    auto setupTables = [&](const auto *locator) {
        for (uint32_t i = 0; i < locator->count(); i++) {
            km::PhysicalAddress paddr = km::PhysicalAddress { locator->entries[i] };
            const acpi::RsdtHeader *header = MapTableEntry(paddr, memory);

            SetUniqueTableEntry(&mMadt, header, "APIC"_sv);
            SetUniqueTableEntry(&mMcfg, header, "MCFG"_sv);
            SetUniqueTableEntry(&mFadt, header, "FACP"_sv);
        }
    };

    if (revision() == 0) {
        const acpi::Rsdt *rsdt = memory.hhdmMapObject<acpi::Rsdt>(km::PhysicalAddress { locator->rsdtAddress });
        setupTables(rsdt);
    } else {
        const acpi::Xsdt *xsdt = memory.hhdmMapObject<acpi::Xsdt>(km::PhysicalAddress { locator->xsdtAddress });
        setupTables(xsdt);
    }
}

uint32_t acpi::AcpiTables::lapicCount() const {
    return std::count_if(mMadt->begin(), mMadt->end(), [](const acpi::MadtEntry *entry) {
        return entry->type == acpi::MadtEntryType::eLocalApic;
    });
}

km::IoApic acpi::AcpiTables::mapIoApic(km::SystemMemory& memory, uint32_t index) const {
    for (const acpi::MadtEntry *entry : *mMadt) {
        if (entry->type == acpi::MadtEntryType::eIoApic) {
            if (index == 0) {
                return km::IoApic { entry, memory };
            }

            index -= 1;
        }
    }

    return km::IoApic { };
}

uint32_t acpi::AcpiTables::ioApicCount() const {
    return std::count_if(mMadt->begin(), mMadt->end(), [](const acpi::MadtEntry *entry) {
        return entry->type == acpi::MadtEntryType::eIoApic;
    });
}

bool acpi::AcpiTables::has8042Controller() const {
    return bool(mFadt->iapcBootArch & acpi::IapcBootArch::e8042Controller);
}

using IapcFormat = km::StaticFormat<acpi::IapcBootArch>;
using AddrSpaceFormat = km::StaticFormat<acpi::AddressSpaceId>;
using AccessFormat = km::StaticFormat<acpi::AccessSize>;
using AddressFormat = km::StaticFormat<acpi::GenericAddress>;

IapcFormat::String IapcFormat::toString(acpi::IapcBootArch arch) {
    IapcFormat::String result;
    bool first = true;

    if (bool(arch & acpi::IapcBootArch::eLegacyDevices)) {
        result.add("Legacy devices"_sv);
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::e8042Controller)) {
        if (!first) {
            result.add(", "_sv);
        }

        result.add("8042 controller"_sv);
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::eVgaNotPresent)) {
        if (!first) {
            result.add(", "_sv);
        }

        result.add("VGA not present"_sv);
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::eMsiNotPresent)) {
        if (!first) {
            result.add(", "_sv);
        }

        result.add("MSI not supported"_sv);
        first = false;
    }

    if (first) {
        return km::format(km::Hex(std::to_underlying(arch)).pad(2, '0'));
    }

    return result;
}

AddrSpaceFormat::String AddrSpaceFormat::toString(acpi::AddressSpaceId space) {
    switch (space) {
    case acpi::AddressSpaceId::eSystemMemory:
        return "RAM"_sv;
    case acpi::AddressSpaceId::eSystemIo:
        return "IO"_sv;
    case acpi::AddressSpaceId::ePciConfig:
        return "PCI"_sv;
    case acpi::AddressSpaceId::eEmbeddedController:
        return "IC"_sv;
    case acpi::AddressSpaceId::eSmbus:
        return "SMB"_sv;
    case acpi::AddressSpaceId::eSystemCmos:
        return "CMOS"_sv;
    case acpi::AddressSpaceId::ePciBarTarget:
        return "BAR"_sv;
    case acpi::AddressSpaceId::eIpmi:
        return "IPMI"_sv;
    case acpi::AddressSpaceId::eGeneralPurposeIo:
        return "GPIO"_sv;
    case acpi::AddressSpaceId::eGenericSerialBus:
        return "UART"_sv;
    case acpi::AddressSpaceId::ePlatformCommChannel:
        return "PCC"_sv;
    case acpi::AddressSpaceId::eFunctionalFixedHardware:
        return "FFH"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(space)).pad(2, '0'));
    }
}

AccessFormat::String AccessFormat::toString(acpi::AccessSize size) {
    switch (size) {
    case acpi::AccessSize::eByte:
        return "Byte"_sv;
    case acpi::AccessSize::eWord:
        return "Word"_sv;
    case acpi::AccessSize::eDword:
        return "Dword"_sv;
    case acpi::AccessSize::eQword:
        return "Qword"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(size)).pad(2, '0'));
    }
}

AddressFormat::String AddressFormat::toString(acpi::GenericAddress addr) {
    AddressFormat::String result;
    auto as = km::format(addr.addressSpace);
    for (ssize_t i = 0; i < 4 - as.count(); i++) {
        result.add(" "_sv);
    }
    result.add(as);
    result.add(":"_sv);
    result.add(km::format(km::PhysicalAddress(addr.address)));

    result.add("+"_sv);
    result.add(km::format(km::Int(addr.offset).pad(3, '0')));

    result.add("["_sv);
    result.add(km::format(km::Int(addr.width).pad(3, '0')));
    result.add("] "_sv);
    result.add(km::format(addr.accessSize));

    return result;
}
