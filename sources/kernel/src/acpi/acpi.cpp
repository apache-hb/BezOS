#include "acpi/acpi.hpp"

#include "acpi/hpet.hpp"
#include "apic.hpp"
#include "logger/logger.hpp"
#include "panic.hpp"
#include "memory/address_space.hpp"
#include "common/util/defer.hpp"

#include <stddef.h>

using namespace stdx::literals;

static constinit km::Logger AcpiLog { "ACPI" };

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
        AcpiLog.warnf("Unknown RSDP revision: ", rsdp->revision, ". Interpreting as an XSDT.");
    case 2:
        return ValidateChecksum((const uint8_t*)rsdp, rsdp->length);
    }
}

static const acpi::RsdtHeader *MapTableEntry(sm::PhysicalAddress paddr, km::AddressSpace& memory) {
    km::TlsfAllocation headerAllocation;

    // first map the header
    const acpi::RsdtHeader *header = memory.mapConst<acpi::RsdtHeader>(paddr, &headerAllocation);
    uint32_t length = header->length;

    if (OsStatus status = memory.unmap(headerAllocation)) {
        AcpiLog.warnf("Failed to unmap RSDT header: ", status);
        AcpiLog.warnf("This is not a fatal error, but the mapping at ", (void*)header, " has been leaked.");
    }

    km::TlsfAllocation tableAllocation; // TODO: this leaks

    // then use the headers length to ensure we map the entire table
    return memory.mapConst<acpi::RsdtHeader>(km::MemoryRangeEx::of(paddr, length), &tableAllocation);
}

static void DebugMadt(const acpi::Madt *madt) {
    acpi::Madt table = *madt;

    AcpiLog.println("| /SYS/ACPI/APIC     | Local APIC address          | ", km::Hex(table.localApicAddress).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/APIC     | Flags                       | ", bool(table.flags & acpi::MadtFlags::ePcatCompat) ? stdx::StringView("PCAT compatible") : stdx::StringView("None"));

    uint32_t index = 0;
    for (const acpi::MadtEntry *entry : table) {
        AcpiLog.println("| /SYS/ACPI/APIC/", km::Int(index).pad(3), " | Entry type                  | ", auto{entry->type});
        AcpiLog.println("| /SYS/ACPI/APIC/", km::Int(index).pad(3), " | Entry length                | ", auto{entry->length});

        index += 1;
    }
}

static void DebugMcfg(const acpi::Mcfg *mcfg) {
    for (size_t i = 0; i < mcfg->allocationCount(); i++) {
        acpi::McfgAllocation allocation = mcfg->allocations[i];
        AcpiLog.println("| /SYS/ACPI/MCFG/", km::Int(i).pad(3), " | Address                     | ", km::PhysicalAddress(allocation.address));
        AcpiLog.println("| /SYS/ACPI/MCFG/", km::Int(i).pad(3), " | PCI Segment                 | ", km::Hex(allocation.segment).pad(4, '0'));
        AcpiLog.println("| /SYS/ACPI/MCFG/", km::Int(i).pad(3), " | Bus range                   | ",
            km::Hex(allocation.startBusNumber).pad(2), "..", km::Hex(allocation.endBusNumber).pad(2));
    }
}

static void DebugFadt(const acpi::Fadt *fadt) {
    acpi::Fadt table = *fadt;

    AcpiLog.println("| /SYS/ACPI/FACP     | Firmware control            | ", km::Hex(table.firmwareCtrl).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | DSDT                        | ", km::Hex(table.dsdt).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | Preferred PM profile        | ", table.preferredPmProfile);
    AcpiLog.println("| /SYS/ACPI/FACP     | SCI interrupt               | ", table.sciInt);
    AcpiLog.println("| /SYS/ACPI/FACP     | SMI command                 | ", km::Hex(table.smiCmd).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | ACPI enable                 | ", table.acpiEnable);
    AcpiLog.println("| /SYS/ACPI/FACP     | ACPI disable                | ", table.acpiDisable);
    AcpiLog.println("| /SYS/ACPI/FACP     | S4 BIOS request             | ", table.s4BiosReq);
    AcpiLog.println("| /SYS/ACPI/FACP     | P-state control             | ", table.pstateCnt);
    AcpiLog.println("| /SYS/ACPI/FACP     | PM1A event block            | ", km::Hex(table.pm1aEvtBlk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | PM1B event block            | ", km::Hex(table.pm1bEvtBlk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | PM1A control block          | ", km::Hex(table.pm1aCntBlk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | PM1B control block          | ", km::Hex(table.pm1bCntBlk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | PM2 control block           | ", km::Hex(table.pm2CntBlk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | PM timer block              | ", km::Hex(table.pmTmrBlk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | GPE0 block                  | ", km::Hex(table.gpe0Blk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | GPE1 block                  | ", km::Hex(table.gpe1Blk).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | PM1 event length            | ", table.pm1EvtLen);
    AcpiLog.println("| /SYS/ACPI/FACP     | PM1 control length          | ", table.pm1CntLen);
    AcpiLog.println("| /SYS/ACPI/FACP     | PM2 control length          | ", table.pm2CntLen);
    AcpiLog.println("| /SYS/ACPI/FACP     | PM timer length             | ", table.pmTmrLen);
    AcpiLog.println("| /SYS/ACPI/FACP     | GPE0 block length           | ", table.gpe0BlkLen);
    AcpiLog.println("| /SYS/ACPI/FACP     | GPE1 block length           | ", table.gpe1BlkLen);
    AcpiLog.println("| /SYS/ACPI/FACP     | GPE1 base                   | ", table.gpe1Base);
    AcpiLog.println("| /SYS/ACPI/FACP     | C-state control             | ", table.cstateCtl);
    AcpiLog.println("| /SYS/ACPI/FACP     | Worst C2 latency            | ", table.worstC2Latency);
    AcpiLog.println("| /SYS/ACPI/FACP     | Worst C3 latency            | ", table.worstC3Latency);
    AcpiLog.println("| /SYS/ACPI/FACP     | Flush size                  | ", table.flushSize);
    AcpiLog.println("| /SYS/ACPI/FACP     | Flush stride                | ", table.flushStride);
    AcpiLog.println("| /SYS/ACPI/FACP     | Duty offset                 | ", table.dutyOffset);
    AcpiLog.println("| /SYS/ACPI/FACP     | Duty width                  | ", table.dutyWidth);
    AcpiLog.println("| /SYS/ACPI/FACP     | Day alarm                   | ", table.dayAlrm);
    AcpiLog.println("| /SYS/ACPI/FACP     | Month alarm                 | ", table.monAlrm);
    AcpiLog.println("| /SYS/ACPI/FACP     | Century                     | ", table.century);
    AcpiLog.println("| /SYS/ACPI/FACP     | IAPC boot arch              | ", auto{table.iapcBootArch});
    AcpiLog.println("| /SYS/ACPI/FACP     | Flags                       | ", km::Hex(table.flags).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | Reset register              | ", table.resetReg);
    AcpiLog.println("| /SYS/ACPI/FACP     | Reset value                 | ", km::Hex(table.resetValue).pad(2, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | ARM boot arch               | ", km::Hex(table.armBootArch).pad(4, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | FADT minor version          | ", table.fadtMinorVersion);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended firmware control   | ", km::Hex(table.x_firmwareCtrl).pad(16, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended DSDT               | ", km::Hex(table.x_dsdt).pad(16, '0'));
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended PM1A event block   | ", table.x_pm1aEvtBlk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended PM1B event block   | ", table.x_pm1bEvtBlk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended PM1A control block | ", table.x_pm1aCntBlk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended PM1B control block | ", table.x_pm1bCntBlk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended PM2 control block  | ", table.x_pm2CntBlk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended PM timer block     | ", table.x_pmTmrBlk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended GPE0 block         | ", table.x_gpe0Blk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Extended GPE1 block         | ", table.x_gpe1Blk);
    AcpiLog.println("| /SYS/ACPI/FACP     | Sleep control register      | ", table.sleepControlReg);
    AcpiLog.println("| /SYS/ACPI/FACP     | Sleep status register       | ", table.sleepStatusReg);
    AcpiLog.println("| /SYS/ACPI/FACP     | Hypervisor vendor ID        | ", km::Hex(table.hypervisorVendor).pad(16));
}

static void DebugHpet(const acpi::Hpet *hpet) {
    acpi::Hpet table = *hpet;

    AcpiLog.println("| /SYS/ACPI/HPET     | Event timer block ID        | ", km::Hex(table.evtTimerBlockId).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/HPET     | Base address                | ", table.baseAddress);
    AcpiLog.println("| /SYS/ACPI/HPET     | HPET number                 | ", table.hpetNumber);
    AcpiLog.println("| /SYS/ACPI/HPET     | Clock tick                  | ", auto{table.clockTick});
    AcpiLog.println("| /SYS/ACPI/HPET     | Page protection             | ", table.pageProtection);
}

static void PrintRsdtEntry(const acpi::RsdtHeader *entry, sm::PhysicalAddress paddr) {
    acpi::RsdtHeader table = *entry;
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | Address                     | ", paddr);
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | Signature                   | '", stdx::StringView(table.signature), "'");
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | Length                      | ", table.length);
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | Revision                    | ", table.revision);
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | OEM                         | ", stdx::StringView(table.oemid));
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | Table ID                    | ", stdx::StringView(table.tableId));
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | OEM revision                | ", table.oemRevision);
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | Creator ID                  | ", km::Hex(table.creatorId));
    AcpiLog.println("| /SYS/ACPI/", table.signature, "     | Creator revision            | ", table.creatorRevision);

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
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(entry), table.length)));
#endif
}

static void PrintRsdt(const acpi::Rsdt *rsdt, const acpi::RsdpLocator *locator) {
    AcpiLog.println("| /SYS/ACPI          | RSDT address                | ", km::Hex(locator->rsdtAddress).pad(8, '0'));
    AcpiLog.println("| /SYS/ACPI/RSDT     | Signature                   | '", stdx::StringView(rsdt->header.signature), "'");
}

static void PrintXsdt(const acpi::Xsdt *xsdt, const acpi::RsdpLocator *locator) {
    AcpiLog.println("| /SYS/ACPI          | RSDP length                 | ", locator->length);
    AcpiLog.println("| /SYS/ACPI          | XSDT address                | ", km::Hex(locator->xsdtAddress).pad(16, '0'));
    AcpiLog.println("| /SYS/ACPI          | Extended checksum           | ", locator->extendedChecksum);
    AcpiLog.println("| /SYS/ACPI/XSDT     | Signature                   | '", stdx::StringView(xsdt->header.signature), "'");
}

acpi::AcpiTables acpi::InitAcpi(sm::PhysicalAddress rsdpBaseAddress, km::AddressSpace& memory) {
    km::TlsfAllocation rsdpAllocation;

    // map the rsdp table
    const acpi::RsdpLocator *locator = memory.mapConst<acpi::RsdpLocator>(std::bit_cast<sm::PhysicalAddress>(rsdpBaseAddress), &rsdpAllocation);

    // validate that the table is ok to use
    bool rsdpOk = ValidateRsdpLocator(locator);
    KM_CHECK(rsdpOk, "Invalid RSDP checksum.");

    AcpiLog.println("| /SYS/ACPI          | RSDP signature              | '", stdx::StringView(locator->signature), "'");
    AcpiLog.println("| /SYS/ACPI          | RSDP checksum               | ", rsdpOk ? stdx::StringView("Valid") : stdx::StringView("Invalid"));
    AcpiLog.println("| /SYS/ACPI          | RSDP revision               | ", locator->revision);
    AcpiLog.println("| /SYS/ACPI          | OEM                         | ", stdx::StringView(locator->oemid));

    return acpi::AcpiTables(rsdpAllocation, locator, memory);
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
            AcpiLog.warnf("[ACPI] Multiple '", T::kSignature, "' tables found. This table should be unique. Using the first instance of this table.");
            return;
        }

        *dst = reinterpret_cast<const T*>(header);
    }
}

acpi::AcpiTables::AcpiTables(km::TlsfAllocation allocation, const RsdpLocator *locator, km::AddressSpace& memory)
    : mRsdpAllocation(allocation)
    , mRsdpLocator(locator)
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
            sm::PhysicalAddress paddr = sm::PhysicalAddress { locator->entries[i] };
            const acpi::RsdtHeader *header = MapTableEntry(paddr, memory);

            SetUniqueTableEntry(&mMadt, header);
            SetUniqueTableEntry(&mMcfg, header);
            SetUniqueTableEntry(&mFadt, header);

            mRsdtEntries[i] = header;

            PrintRsdtEntry(header, paddr);
        }
    };

    km::TlsfAllocation rsdtAllocation;
    defer { KM_ASSERT(memory.unmap(rsdtAllocation) == OsStatusSuccess); };

    if (revision() == 0) {
        const acpi::Rsdt *rsdt = memory.mapConst<acpi::Rsdt>(sm::PhysicalAddress { locator->rsdtAddress }, &rsdtAllocation);
        PrintRsdt(rsdt, locator);
        setupTables(rsdt);
    } else {
        const acpi::Xsdt *xsdt = memory.mapConst<acpi::Xsdt>(sm::PhysicalAddress { locator->xsdtAddress }, &rsdtAllocation);
        PrintXsdt(xsdt, locator);
        setupTables(xsdt);
    }

    if (mFadt != nullptr) {
        uint64_t address = (revision() == 0) ? mFadt->dsdt : mFadt->x_dsdt;
        mDsdt = MapTableEntry(sm::PhysicalAddress { address }, memory);
    }

#if 0
    if (mDsdt != nullptr) {
        AcpiLog.println(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(mDsdt), mDsdt->length)));
    }
#endif
}

uint32_t acpi::AcpiTables::lapicCount() const {
    return mMadt->lapicCount();
}

km::IoApic acpi::AcpiTables::mapIoApic(km::AddressSpace& memory, uint32_t index) const {
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
