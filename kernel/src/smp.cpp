#include "smp.hpp"

#include "arch/paging.hpp"
#include "gdt.hpp"

struct [[gnu::packed]] SmpInfoHeader {
    uint64_t startAddress;

    alignas(16) uint64_t gdtEntries[7];
    alignas(16) GDTR gdtr;
};

extern const char _binary_smp_start[];
extern const char _binary_smp_end[];

static uintptr_t GetSmpBlobSize(void) {
    return (uintptr_t)_binary_smp_end - (uintptr_t)_binary_smp_start;
}

static constexpr km::PhysicalAddress kSmpInfo = 0x7000;
static constexpr km::PhysicalAddress kSmpStart = 0x8000;

extern "C" [[noreturn]] void KmSmpStartup(void) {
    KmDebugMessage("[SMP] Starting Core.\n");
    KmHalt();
}

void KmInitSmp(km::SystemMemory& memory, km::LocalAPIC& bsp, acpi::AcpiTables& acpiTables) {
    uint32_t bspId = bsp.id();
    size_t blobSize = GetSmpBlobSize();
    void *smpStartBlob = memory.hhdmMap(kSmpStart, kSmpStart + blobSize);
    memcpy(smpStartBlob, _binary_smp_start, blobSize);

    void *smpInfo = memory.hhdmMap(kSmpInfo, kSmpInfo + km::pages(0x1000));
    SmpInfoHeader header = {
        .startAddress = (uintptr_t)KmSmpStartup - memory.pager.hhdmOffset(),
        .gdtEntries = {
            // Null descriptor
            x64::kNullDescriptor,

            // Real mode code segment
            x64::BuildSegmentDescriptor(
                x64::DescriptorFlags::eNone,
                x64::SegmentAccessFlags::eCodeOrDataSegment | x64::SegmentAccessFlags::eReadWrite | x64::SegmentAccessFlags::eExecutable | x64::SegmentAccessFlags::ePresent | x64::SegmentAccessFlags::eAccessed,
                0xFFFF
            ),

            // Real mode data segment
            x64::BuildSegmentDescriptor(
                x64::DescriptorFlags::eNone,
                x64::SegmentAccessFlags::eCodeOrDataSegment | x64::SegmentAccessFlags::eReadWrite | x64::SegmentAccessFlags::ePresent | x64::SegmentAccessFlags::eAccessed,
                0xFFFF
            ),

            // Protected mode code segment
            x64::BuildSegmentDescriptor(
                x64::DescriptorFlags::eSize | x64::DescriptorFlags::eGranularity,
                x64::SegmentAccessFlags::eCodeOrDataSegment | x64::SegmentAccessFlags::eReadWrite | x64::SegmentAccessFlags::eExecutable | x64::SegmentAccessFlags::eRing0 | x64::SegmentAccessFlags::ePresent | x64::SegmentAccessFlags::eAccessed,
                0xFFFFF
            ),

            // Protected mode data segment
            x64::BuildSegmentDescriptor(
                x64::DescriptorFlags::eSize | x64::DescriptorFlags::eGranularity,
                x64::SegmentAccessFlags::eCodeOrDataSegment | x64::SegmentAccessFlags::eReadWrite | x64::SegmentAccessFlags::eRing0 | x64::SegmentAccessFlags::eReadWrite | x64::SegmentAccessFlags::ePresent | x64::SegmentAccessFlags::eAccessed,
                0xFFFFF
            ),
        },
        .gdtr = {
            .limit = sizeof(header.gdtEntries) - 1,
            .base = offsetof(SmpInfoHeader, gdtEntries) + kSmpInfo.address,
        }
    };
    memcpy(smpInfo, &header, sizeof(header));

    for (const acpi::MadtEntry *madt : *acpiTables.madt()) {
        if (madt->type == acpi::MadtEntryType::eLocalApic) {
            const acpi::MadtEntry::LocalApic localApic = madt->lapic;

            // No need to start the BSP
            if (localApic.apicId == bspId)
                continue;

            if (!localApic.isEnabled() && !localApic.isOnlineCapable()) {
                KmDebugMessage("[SMP] Skipping APIC ID: ", localApic.apicId, ", core is reported as damaged.\n");
                continue;
            }

            KmDebugMessage("[SMP] Starting APIC ID: ", localApic.apicId, "\n");

            // Send the INIT IPI
            bsp.sendIpi(localApic.apicId, 0x4500);

            // TODO: sleep

            // Send the start IPI
            size_t smpStartAddress = kSmpStart.address / x64::kPageSize;
            bsp.sendIpi(localApic.apicId, 0x4600 | smpStartAddress);

            // TODO: sleep again
        }
    }
}
