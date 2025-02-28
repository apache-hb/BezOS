#include "acpi/acpi.hpp"

#include "acpi/hpet.hpp"
#include "apic.hpp"
#include "log.hpp"
#include "panic.hpp"

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
    const acpi::RsdtHeader *header = memory.mapConst<acpi::RsdtHeader>(paddr);
    uint32_t length = header->length;
    memory.unmap((void*)header, sizeof(acpi::RsdtHeader));

    // then use the headers length to ensure we map the entire table
    return memory.mapConst<acpi::RsdtHeader>(km::MemoryRange::of(paddr, length));
}

static void DebugMadt(const acpi::Madt *madt) {
    acpi::Madt table = *madt;

    KmDebugMessage("| /SYS/ACPI/APIC     | Local APIC address          | ", km::Hex(table.localApicAddress).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/APIC     | Flags                       | ", bool(table.flags & acpi::MadtFlags::ePcatCompat) ? stdx::StringView("PCAT compatible") : stdx::StringView("None"), "\n");

    uint32_t index = 0;
    for (const acpi::MadtEntry *entry : table) {
        KmDebugMessage("| /SYS/ACPI/APIC/", km::rpad(3) + index, " | Entry type                  | ", auto{entry->type}, "\n");
        KmDebugMessage("| /SYS/ACPI/APIC/", km::rpad(3) + index, " | Entry length                | ", auto{entry->length}, "\n");

        index += 1;
    }
}

static void DebugMcfg(const acpi::Mcfg *mcfg) {
    for (size_t i = 0; i < mcfg->allocationCount(); i++) {
        acpi::McfgAllocation allocation = mcfg->allocations[i];
        KmDebugMessage("| /SYS/ACPI/MCFG/", km::rpad(3) + i, " | Address                     | ", km::PhysicalAddress(allocation.address), "\n");
        KmDebugMessage("| /SYS/ACPI/MCFG/", km::rpad(3) + i, " | PCI Segment                 | ", km::Hex(allocation.segment).pad(4, '0'), "\n");
        KmDebugMessage("| /SYS/ACPI/MCFG/", km::rpad(3) + i, " | Bus range                   | ",
            km::Hex(allocation.startBusNumber).pad(2, '0'), "..", km::Hex(allocation.endBusNumber).pad(2, '0'), "\n");
    }
}

static void DebugFadt(const acpi::Fadt *fadt) {
    acpi::Fadt table = *fadt;

    KmDebugMessage("| /SYS/ACPI/FACP     | Firmware control            | ", km::Hex(table.firmwareCtrl).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | DSDT                        | ", km::Hex(table.dsdt).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Preferred PM profile        | ", table.preferredPmProfile, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | SCI interrupt               | ", table.sciInt, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | SMI command                 | ", km::Hex(table.smiCmd).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | ACPI enable                 | ", table.acpiEnable, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | ACPI disable                | ", table.acpiDisable, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | S4 BIOS request             | ", table.s4BiosReq, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | P-state control             | ", table.pstateCnt, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1A event block            | ", km::Hex(table.pm1aEvtBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1B event block            | ", km::Hex(table.pm1bEvtBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1A control block          | ", km::Hex(table.pm1aCntBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1B control block          | ", km::Hex(table.pm1bCntBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM2 control block           | ", km::Hex(table.pm2CntBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM timer block              | ", km::Hex(table.pmTmrBlk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE0 block                  | ", km::Hex(table.gpe0Blk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE1 block                  | ", km::Hex(table.gpe1Blk).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1 event length            | ", table.pm1EvtLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM1 control length          | ", table.pm1CntLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM2 control length          | ", table.pm2CntLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | PM timer length             | ", table.pmTmrLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE0 block length           | ", table.gpe0BlkLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE1 block length           | ", table.gpe1BlkLen, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | GPE1 base                   | ", table.gpe1Base, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | C-state control             | ", table.cstateCtl, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Worst C2 latency            | ", table.worstC2Latency, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Worst C3 latency            | ", table.worstC3Latency, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Flush size                  | ", table.flushSize, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Flush stride                | ", table.flushStride, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Duty offset                 | ", table.dutyOffset, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Duty width                  | ", table.dutyWidth, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Day alarm                   | ", table.dayAlrm, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Month alarm                 | ", table.monAlrm, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Century                     | ", table.century, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | IAPC boot arch              | ", auto{table.iapcBootArch}, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Flags                       | ", km::Hex(table.flags).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Reset register              | ", table.resetReg, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Reset value                 | ", km::Hex(table.resetValue).pad(2, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | ARM boot arch               | ", km::Hex(table.armBootArch).pad(4, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | FADT minor version          | ", table.fadtMinorVersion, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended firmware control   | ", km::Hex(table.x_firmwareCtrl).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended DSDT               | ", km::Hex(table.x_dsdt).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1A event block   | ", table.x_pm1aEvtBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1B event block   | ", table.x_pm1bEvtBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1A control block | ", table.x_pm1aCntBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM1B control block | ", table.x_pm1bCntBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM2 control block  | ", table.x_pm2CntBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended PM timer block     | ", table.x_pmTmrBlk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended GPE0 block         | ", table.x_gpe0Blk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Extended GPE1 block         | ", table.x_gpe1Blk, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Sleep control register      | ", table.sleepControlReg, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Sleep status register       | ", table.sleepStatusReg, "\n");
    KmDebugMessage("| /SYS/ACPI/FACP     | Hypervisor vendor ID        | ", table.hypervisorVendor, "\n");
}

static void DebugHpet(const acpi::Hpet *hpet) {
    acpi::Hpet table = *hpet;

    KmDebugMessage("| /SYS/ACPI/HPET     | Event timer block ID        | ", km::Hex(table.evtTimerBlockId).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/HPET     | Base address                | ", table.baseAddress, "\n");
    KmDebugMessage("| /SYS/ACPI/HPET     | HPET number                 | ", table.hpetNumber, "\n");
    KmDebugMessage("| /SYS/ACPI/HPET     | Clock tick                  | ", auto{table.clockTick}, "\n");
    KmDebugMessage("| /SYS/ACPI/HPET     | Page protection             | ", table.pageProtection, "\n");
}

static void PrintRsdtEntry(const acpi::RsdtHeader *entry, km::PhysicalAddress paddr) {
    acpi::RsdtHeader table = *entry;
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | Address                     | ", paddr, "\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | Signature                   | '", stdx::StringView(table.signature), "'\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | Length                      | ", table.length, "\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | Revision                    | ", table.revision, "\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | OEM                         | ", stdx::StringView(table.oemid), "\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | Table ID                    | ", stdx::StringView(table.tableId), "\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | OEM revision                | ", table.oemRevision, "\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | Creator ID                  | ", km::Hex(table.creatorId), "\n");
    KmDebugMessage("| /SYS/ACPI/", table.signature, "     | Creator revision            | ", table.creatorRevision, "\n");

    if (auto *madt = acpi::TableCast<acpi::Madt>(entry)) {
        DebugMadt(madt);
    } else if (auto *mcfg = acpi::TableCast<acpi::Mcfg>(entry)) {
        DebugMcfg(mcfg);
    } else if (auto *fadt = acpi::TableCast<acpi::Fadt>(entry)) {
        DebugFadt(fadt);
    } else if (auto *hpet = acpi::TableCast<acpi::Hpet>(entry)) {
        DebugHpet(hpet);
    }

#if 0
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(entry), table.length)), "\n");
#endif
}

static void PrintRsdt(const acpi::Rsdt *rsdt, const acpi::RsdpLocator *locator) {
    KmDebugMessage("| /SYS/ACPI          | RSDT address                | ", km::Hex(locator->rsdtAddress).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/RSDT     | Signature                   | '", stdx::StringView(rsdt->header.signature), "'\n");
}

static void PrintXsdt(const acpi::Xsdt *xsdt, const acpi::RsdpLocator *locator) {
    KmDebugMessage("| /SYS/ACPI          | RSDP length                 | ", locator->length, "\n");
    KmDebugMessage("| /SYS/ACPI          | XSDT address                | ", km::Hex(locator->xsdtAddress).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI          | Extended checksum           | ", locator->extendedChecksum, "\n");
    KmDebugMessage("| /SYS/ACPI/XSDT     | Signature                   | '", stdx::StringView(xsdt->header.signature), "'\n");
}

acpi::AcpiTables acpi::InitAcpi(km::PhysicalAddress rsdpBaseAddress, km::SystemMemory& memory) {
    // map the rsdp table
    const acpi::RsdpLocator *locator = memory.mapConst<acpi::RsdpLocator>(rsdpBaseAddress);

    // validate that the table is ok to use
    bool rsdpOk = ValidateRsdpLocator(locator);
    KM_CHECK(rsdpOk, "Invalid RSDP checksum.");

    KmDebugMessage("| /SYS/ACPI          | RSDP signature              | '", stdx::StringView(locator->signature), "'\n");
    KmDebugMessage("| /SYS/ACPI          | RSDP checksum               | ", rsdpOk ? stdx::StringView("Valid") : stdx::StringView("Invalid"), "\n");
    KmDebugMessage("| /SYS/ACPI          | RSDP revision               | ", locator->revision, "\n");
    KmDebugMessage("| /SYS/ACPI          | OEM                         | ", stdx::StringView(locator->oemid), "\n");

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

template<acpi::IsAcpiTable T>
void SetUniqueTableEntry(const T** dst, const acpi::RsdtHeader *header) {
    if (const T *table = acpi::TableCast<T>(header)) {
        if (*dst != nullptr) {
            KmDebugMessage("[ACPI] Multiple '", T::kSignature, "' tables found. This table should be unique. I will only use the first instance of this table.\n");
            return;
        }

        *dst = reinterpret_cast<const T*>(header);
    }
}

acpi::AcpiTables::AcpiTables(const RsdpLocator *locator, km::SystemMemory& memory)
    : mRsdpLocator(locator)
    , mMadt(nullptr)
    , mMcfg(nullptr)
    , mFadt(nullptr)
    , mDsdt(nullptr)
{
    auto setupTables = [&](const auto *locator) {
        mRsdtEntryCount = locator->count();

        mRsdtEntries.reset(new (std::nothrow) const RsdtHeader*[mRsdtEntryCount]);
        KM_CHECK(mRsdtEntries != nullptr, "Failed to allocate memory for RSDT entries.");

        for (uint32_t i = 0; i < mRsdtEntryCount; i++) {
            km::PhysicalAddress paddr = km::PhysicalAddress { locator->entries[i] };
            const acpi::RsdtHeader *header = MapTableEntry(paddr, memory);

            SetUniqueTableEntry(&mMadt, header);
            SetUniqueTableEntry(&mMcfg, header);
            SetUniqueTableEntry(&mFadt, header);

            mRsdtEntries[i] = header;

            PrintRsdtEntry(header, paddr);
        }
    };

    if (revision() == 0) {
        const acpi::Rsdt *rsdt = memory.mapConst<acpi::Rsdt>(km::PhysicalAddress { locator->rsdtAddress });
        PrintRsdt(rsdt, locator);
        setupTables(rsdt);
    } else {
        const acpi::Xsdt *xsdt = memory.mapConst<acpi::Xsdt>(km::PhysicalAddress { locator->xsdtAddress });
        PrintXsdt(xsdt, locator);
        setupTables(xsdt);
    }

    if (mFadt != nullptr) {
        uint64_t address = (revision() == 0) ? mFadt->dsdt : mFadt->x_dsdt;
        mDsdt = MapTableEntry(km::PhysicalAddress { address }, memory);
    }

#if 0
    if (mDsdt != nullptr) {
        KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(mDsdt), mDsdt->length)), "\n");
    }
#endif
}

uint32_t acpi::AcpiTables::lapicCount() const {
    return mMadt->lapicCount();
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
    return mMadt->ioApicCount();
}

bool acpi::AcpiTables::has8042Controller() const {
    return bool(mFadt->iapcBootArch & acpi::IapcBootArch::e8042Controller);
}

uint32_t acpi::Madt::ioApicCount() const {
    return std::count_if(begin(), end(), [](const MadtEntry *entry) {
        return entry->type == MadtEntryType::eIoApic;
    });
}

uint32_t acpi::Madt::lapicCount() const {
    return std::count_if(begin(), end(), [](const acpi::MadtEntry *entry) {
        return entry->type == acpi::MadtEntryType::eLocalApic;
    });
}
