#include "acpi.hpp"
#include "apic.hpp"
#include "kernel.hpp"

#include "memory/layout.hpp"

#include <stddef.h>

using namespace stdx::literals;

static bool KmValidateChecksum(const uint8_t *bytes, size_t length) {
    uint32_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }

    return (sum & 0xFF) == 0;
}

static bool KmValidateRsdpLocator(const acpi::RsdpLocator *rsdp) {
    switch (rsdp->revision) {
    case 0:
        return KmValidateChecksum((const uint8_t*)rsdp, 20);
    default:
        KmDebugMessage("[INIT] Unknown RSDP revision: ", rsdp->revision, ". Doing best guess validation.\n");
    case 2:
        return KmValidateChecksum((const uint8_t*)rsdp, rsdp->length);
    }
}

static acpi::RsdtHeader *KmMapTableEntry(km::PhysicalAddress paddr, km::SystemMemory& memory) {
    // first map the header
    acpi::RsdtHeader *header = memory.hhdmMapObject<acpi::RsdtHeader>(paddr);

    // then use the headers length to ensure we map the entire table
    return memory.hhdmMapObject<acpi::RsdtHeader>(paddr, paddr + header->length);
}

static void KmDebugMadt(const acpi::RsdtHeader *header) {
    const acpi::Madt *madt = reinterpret_cast<const acpi::Madt*>(header);

    KmDebugMessage("| /SYS/ACPI/APIC     | Local APIC address   | ", km::Hex(madt->localApicAddress).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI/APIC     | Flags                | ", bool(madt->flags & acpi::MadtFlags::ePcatCompat) ? stdx::StringView("PCAT compatible") : stdx::StringView("None"), "\n");

    uint32_t index = 0;
    for (const acpi::MadtEntry *entry : *madt) {
        KmDebugMessage("| /SYS/ACPI/APIC/", km::rpad(3) + index, " | Entry type           | ", entry->type, "\n");
        KmDebugMessage("| /SYS/ACPI/APIC/", km::rpad(3) + index, " | Entry length         | ", entry->length, "\n");

        index += 1;
    }
}

static acpi::RsdtHeader *KmGetRsdtHeader(km::PhysicalAddress paddr, km::SystemMemory& memory) {
    acpi::RsdtHeader *entry = KmMapTableEntry(paddr, memory);
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Address              | ", paddr, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Signature            | '", stdx::StringView(entry->signature), "'\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Length               | ", entry->length, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Revision             | ", entry->revision, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | OEM                  | ", stdx::StringView(entry->oemid), "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Table ID             | ", stdx::StringView(entry->tableId), "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | OEM revision         | ", entry->oemRevision, "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Creator ID           | ", km::Hex(entry->creatorId), "\n");
    KmDebugMessage("| /SYS/ACPI/", entry->signature, "     | Creator revision     | ", entry->creatorRevision, "\n");

    if (stdx::StringView(entry->signature) == "APIC"_sv) {
        KmDebugMadt(entry);
    }

    return entry;
}

static void KmDebugRSDT(const acpi::RsdpLocator *locator, km::SystemMemory& memory) {
    KmDebugMessage("| /SYS/ACPI          | RSDT address         | ", km::Hex(locator->rsdtAddress).pad(8, '0'), "\n");

    acpi::Rsdt *rsdt = memory.hhdmMapObject<acpi::Rsdt>(locator->rsdtAddress);

    KmDebugMessage("| /SYS/ACPI/RSDT     | Signature            | '", stdx::StringView(rsdt->header.signature), "'\n");

    for (uint32_t i = 0; i < rsdt->count(); i++) {
        km::PhysicalAddress paddr = km::PhysicalAddress { rsdt->entries[i] };
        [[maybe_unused]]
        acpi::RsdtHeader *entry = KmGetRsdtHeader(paddr, memory);
    }
}

static void KmDebugXSDT(const acpi::RsdpLocator *locator, km::SystemMemory& memory) {
    KmDebugMessage("| /SYS/ACPI          | RSDP length          | ", locator->length, "\n");
    KmDebugMessage("| /SYS/ACPI          | XSDT address         | ", km::Hex(locator->xsdtAddress).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/ACPI          | Extended checksum    | ", locator->extendedChecksum, "\n");

    acpi::Xsdt *xsdt = memory.hhdmMapObject<acpi::Xsdt>(km::PhysicalAddress { locator->xsdtAddress });

    KmDebugMessage("| /SYS/ACPI/XSDT     | Signature            | '", stdx::StringView(xsdt->header.signature), "'\n");

    for (uint32_t i = 0; i < xsdt->count(); i++) {
        km::PhysicalAddress paddr = km::PhysicalAddress { xsdt->entries[i] };
        [[maybe_unused]]
        acpi::RsdtHeader *entry = KmGetRsdtHeader(paddr, memory);
    }
}

acpi::AcpiTables KmInitAcpi(km::PhysicalAddress rsdpBaseAddress, km::SystemMemory& memory) {
    // map the rsdp table
    acpi::RsdpLocator *locator = memory.hhdmMapObject<acpi::RsdpLocator>(rsdpBaseAddress);

    // validate that the table is ok to use
    bool rsdpOk = KmValidateRsdpLocator(locator);
    KM_CHECK(rsdpOk, "Invalid RSDP checksum.");

    KmDebugMessage("| /SYS/ACPI          | RSDP signature       | '", stdx::StringView(locator->signature), "'\n");
    KmDebugMessage("| /SYS/ACPI          | RSDP checksum        | ", rsdpOk ? stdx::StringView("Valid") : stdx::StringView("Invalid"), "\n");
    KmDebugMessage("| /SYS/ACPI          | RSDP revision        | ", locator->revision, "\n");
    KmDebugMessage("| /SYS/ACPI          | OEM                  | ", stdx::StringView(locator->oemid), "\n");

    if (locator->revision == 0) {
        KmDebugRSDT(locator, memory);
    } else {
        KmDebugXSDT(locator, memory);
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

acpi::AcpiTables::AcpiTables(const RsdpLocator *locator, km::SystemMemory& memory)
    : mRsdpLocator(locator)
{
    // TODO: duplicated code
    if (revision() == 0) {
        const acpi::Rsdt *rsdt = memory.hhdmMapObject<acpi::Rsdt>(km::PhysicalAddress { locator->rsdtAddress });

        for (uint32_t i = 0; i < rsdt->count(); i++) {
            km::PhysicalAddress paddr = km::PhysicalAddress { rsdt->entries[i] };
            const acpi::RsdtHeader *header = KmMapTableEntry(paddr, memory);
            if (stdx::StringView(header->signature) == "APIC"_sv) {
                mMadt = reinterpret_cast<const acpi::Madt*>(KmMapTableEntry(paddr, memory));
                break;
            }
        }
    } else {
        const acpi::Xsdt *xsdt = memory.hhdmMapObject<acpi::Xsdt>(km::PhysicalAddress { locator->xsdtAddress });

        for (uint32_t i = 0; i < xsdt->count(); i++) {
            km::PhysicalAddress paddr = km::PhysicalAddress { xsdt->entries[i] };
            const acpi::RsdtHeader *header = KmMapTableEntry(paddr, memory);
            if (stdx::StringView(header->signature) == "APIC"_sv) {
                mMadt = reinterpret_cast<const acpi::Madt*>(KmMapTableEntry(paddr, memory));
                break;
            }
        }
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
