#include "smp.hpp"

#include "arch/paging.hpp"

#include "gdt.hpp"
#include "isr.hpp"
#include "pat.hpp"

#include <atomic>

/// @brief The info passed to the smp startup blob.
/// @warning MUST BE KEPT IN SYNC WITH KmSmpInfoStart
struct SmpInfoHeader {
    /// @brief The address of the long mode entry point for the AP.
    /// @note For now this is always the address of @ref KmSmpStartup.
    alignas(uint64_t) uintptr_t startAddress;

    /// @brief The PAT MSR value.
    /// Every core requires the same PAT values when paging is enabled.
    /// @see Intel SDM Vol3A 10-30 - MULTIPLE-PROCESSOR MANAGEMENT 10.7.4
    alignas(uint64_t) uint64_t pat;

    /// @brief The physical address of the kernels top level page table.
    /// @note As this is 32 bit this places a constraint on the pml4 that it must always
    ///       be in the first 4G of memory. It would be ideal to have a seperate
    ///       page heirarchy for ap startup to remove this silent constraint.
    alignas(uint32_t) uint32_t pml4;

    /// @brief The stack pointer for the currently starting AP.
    alignas(uint64_t) uint64_t stack;

    /// @brief The startup GDT used to reach long mode.
    alignas(16) SystemGdt gdt;

    /// @brief The GDTR that will be loaded.
    alignas(16) GDTR gdtr;

    /// Fields after this point are only visible in C++ land and do not need
    /// to line up with smp.S

    /// @brief The ready flag.
    /// Once an AP has been fully started it will set this flag, which signals to
    /// the BSP that the next core can be started safely.
    std::atomic<uint32_t> ready;

    km::LocalApic *bspApic;

    km::SystemMemory *memory;
};

static_assert(offsetof(SmpInfoHeader, gdt) == 32);

static_assert(std::is_standard_layout_v<SmpInfoHeader>, "The SMP header must have the layout smp.S expects.");
static_assert(sizeof(SmpInfoHeader) < 0x1000, "If the SMP header gets this large it will overwrite the BSP startup area.");

extern const char _binary_smp_start[];
extern const char _binary_smp_end[];

static uintptr_t GetSmpBlobSize(void) {
    return (uintptr_t)_binary_smp_end - (uintptr_t)_binary_smp_start;
}

static constexpr km::PhysicalAddress kSmpInfo = 0x7000;
static constexpr km::PhysicalAddress kSmpStart = 0x8000;

extern "C" [[noreturn]] void KmSmpStartup(SmpInfoHeader *header) {
    KmDebugMessage("[SMP] Starting Core.\n");

    KmSetupApGdt();
    KmLoadIdt();

    km::LocalApic lapic = KmInitApLocalApic(*header->memory, *header->bspApic);

    KmDebugMessage("[SMP] Started AP ", lapic.id(), "\n");

    header->ready = 1;

    KmHalt();
}

static uintptr_t AllocSmpStack(km::SystemMemory& memory) {
    km::PhysicalAddress stack = memory.pmm.alloc4k(4);
    return (uintptr_t)memory.map(stack, stack + (x64::kPageSize * 4)) + (x64::kPageSize * 4);
}

static SmpInfoHeader SetupSmpInfoHeader(km::SystemMemory *memory, km::LocalApic *lapic) {
    km::PageTableManager& vmm = memory->vmm;

    km::PhysicalAddress pml4 = vmm.rootPageTable();
    KM_CHECK(pml4 < UINT32_MAX, "PML4 address is above the 4G range.");

    return SmpInfoHeader {
        .startAddress = (uintptr_t)KmSmpStartup,
        .pat = x64::LoadPatMsr(),
        .pml4 = uint32_t(pml4.address),
        .gdt = KmGetBootGdt(),
        .gdtr = {
            .limit = sizeof(SmpInfoHeader::gdt) - 1,
            .base = offsetof(SmpInfoHeader, gdt) + kSmpInfo.address,
        },
        .bspApic = lapic,
        .memory = memory,
    };
}

void KmInitSmp(km::SystemMemory& memory, km::LocalApic& bsp, acpi::AcpiTables& acpiTables) {
    // copy the SMP blob to the correct location
    size_t blobSize = GetSmpBlobSize();
    void *smpStartBlob = memory.map(kSmpStart, kSmpStart + blobSize, km::PageFlags::eAll);
    memcpy(smpStartBlob, _binary_smp_start, blobSize);

    KmDebugMessage("[SMP] Starting APs.\n");

    uint32_t bspId = bsp.id();

    KmDebugMessage("[SMP] BSP ID: ", bspId, "\n");

    SmpInfoHeader *smpInfo = memory.mmapObject<SmpInfoHeader>(kSmpInfo);

    // Also identity map the SMP blob and info regions, it makes jumping to compatibility mode easier.
    // I think theres a better way to do this, but I'm not sure what it is.
    memory.vmm.mapRange({ kSmpInfo, kSmpInfo + sizeof(SmpInfoHeader) }, (void*)kSmpInfo.address, km::PageFlags::eData);
    memory.vmm.mapRange({ kSmpStart, kSmpStart + blobSize }, (void*)kSmpStart.address, km::PageFlags::eCode);

    SmpInfoHeader header = SetupSmpInfoHeader(&memory, &bsp);
    memcpy(smpInfo, &header, sizeof(header));

    for (const acpi::MadtEntry *madt : *acpiTables.madt()) {
        if (madt->type != acpi::MadtEntryType::eLocalApic)
            continue;

        const acpi::MadtEntry::LocalApic localApic = madt->lapic;

        // No need to start the BSP
        if (localApic.apicId == bspId)
            continue;

        if (!localApic.isEnabled() && !localApic.isOnlineCapable()) {
            KmDebugMessage("[SMP] Skipping APIC ID: ", localApic.apicId, ", core is marked as inoperable.\n");
            continue;
        }

        smpInfo->stack = AllocSmpStack(memory);
        smpInfo->ready = 0;

        KmDebugMessage("[SMP] Starting APIC ID: ", localApic.apicId, "\n");

        // Send the INIT IPI
        bsp.sendIpi(localApic.apicId, 0x4500);

        // TODO: sleep

        // Send the start IPI
        size_t smpStartAddress = kSmpStart.address / x64::kPageSize;
        bsp.sendIpi(localApic.apicId, 0x4600 | smpStartAddress);

        // TODO: should really have a condition variable here
        while (!smpInfo->ready) {
            _mm_pause();
            // Spin until the core is ready
        }
    }

    // Now that we're finished, cleanup the smp blob and startup area.

    // Unmap the HHDM mappings
    memory.unmap(smpStartBlob, blobSize);
    memory.unmap(smpInfo, sizeof(SmpInfoHeader));

    // And unmap the identity mappings
    memory.vmm.unmap((void*)kSmpInfo.address, sizeof(SmpInfoHeader));
    memory.vmm.unmap((void*)kSmpStart.address, blobSize);
}
