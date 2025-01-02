#include "smp.hpp"

#include "arch/paging.hpp"
#include "arch/cr0.hpp"
#include "arch/cr4.hpp"
#include "gdt.hpp"
#include "isr.hpp"
#include "pat.hpp"

struct SmpInfoHeader {
    uint64_t startAddress;
    uint64_t pat0;
    uint32_t pml4;
    volatile uint32_t ready;
    uint64_t stack;

    alignas(16) SystemGdt gdt;
    alignas(16) GDTR gdtr;
};

static_assert(std::is_standard_layout_v<SmpInfoHeader>);

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

    header->ready = 1;

    KmHalt();
}

static uintptr_t AllocSmpStack(km::SystemMemory& memory) {
    km::PhysicalAddress stack = memory.pmm.alloc4k(4);
    return (uintptr_t)memory.hhdmMap(stack, stack + (x64::kPageSize * 4)) + (x64::kPageSize * 4);
}

static SmpInfoHeader SetupSmpInfoHeader(km::SystemMemory& memory) {
    uintptr_t pml4 = (uintptr_t)memory.vmm.rootPageTable() - memory.pager.hhdmOffset();
    KM_CHECK(pml4 < UINT32_MAX, "PML4 address is above the 4G range.");

    KmDebugMessage("[SMP] PML4: ", km::Hex(pml4).pad(16, '0'), "\n");

    uint64_t pat = x64::LoadPatMsr();

    KmDebugMessage("[SMP] PAT: ", km::Hex(pat).pad(16, '0'), "\n");

    KmDebugMessage("[INIT] CR0: ", x64::Cr0::load(), "\n");
    KmDebugMessage("[INIT] CR2: ", km::Hex(x64::cr2()).pad(16, '0'), "\n");
    KmDebugMessage("[INIT] CR3: ", km::Hex(x64::cr3()).pad(16, '0'), "\n");
    KmDebugMessage("[INIT] CR4: ", x64::Cr4::load(), "\n");

    return SmpInfoHeader {
        .startAddress = (uintptr_t)KmSmpStartup,
        .pat0 = pat,
        .pml4 = uint32_t(pml4),
        .ready = 0,
        .gdt = KmGetSystemGdt(),
        .gdtr = {
            .limit = sizeof(SmpInfoHeader::gdt) - 1,
            .base = offsetof(SmpInfoHeader, gdt) + kSmpInfo.address,
        }
    };
}

static_assert(sizeof(SystemGdt) == (9 * sizeof(uint64_t)), "Dont change the System GDT size without also changing the SMP header in smp.S");

void KmInitSmp(km::SystemMemory& memory, km::LocalAPIC& bsp, acpi::AcpiTables& acpiTables) {
    // copy the SMP blob to the correct location
    size_t blobSize = GetSmpBlobSize();
    void *smpStartBlob = memory.hhdmMap(kSmpStart, kSmpStart + blobSize, km::PageFlags::eAll);
    memcpy(smpStartBlob, _binary_smp_start, blobSize);

    uint32_t bspId = bsp.id();
    SmpInfoHeader *smpInfo = memory.hhdmMapObject<SmpInfoHeader>(kSmpInfo);

    // Also identity map the SMP blob and info regions, it makes jumping to compatibility mode easier.
    // I think theres a better way to do this, but I'm not sure what it is.
    memory.vmm.mapRange({ kSmpInfo, kSmpInfo + sizeof(SmpInfoHeader) }, kSmpInfo.address, km::PageFlags::eData);
    memory.vmm.mapRange({ kSmpStart, kSmpStart + blobSize }, kSmpStart.address, km::PageFlags::eCode);

    SmpInfoHeader header = SetupSmpInfoHeader(memory);
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

        KmDebugMessage("[SMP] APIC ID: ", localApic.apicId, " started.\n");

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
