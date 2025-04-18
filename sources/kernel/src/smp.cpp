#include "smp.hpp"

#include "isr/isr.hpp"
#include "isr/runtime.hpp"

#include "gdt.hpp"
#include "kernel.hpp"
#include "panic.hpp"
#include "log.hpp"
#include "pat.hpp"
#include "processor.hpp"
#include "syscall.hpp"
#include "thread.hpp"
#include "xsave.hpp"

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
    alignas(16) km::SystemGdt gdt;

    /// @brief The GDTR that will be loaded.
    alignas(16) GDTR gdtr;

    /// Fields after this point are only visible in C++ land and do not need
    /// to line up with smp.S

    /// @brief The ready flag.
    /// Once an AP has been fully started it will set this flag, which signals to
    /// the BSP that the next core can be started safely.
    std::atomic_flag ready;

    km::IApic *bspApic;
    km::SystemMemory *memory;
    km::SmpInitCallback callback;
    void *user;
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

    km::SetupInitialGdt();
    km::LoadIdt();

    km::Apic apic = km::InitApApic(header->memory->pageTables(), header->bspApic);

    km::InitCpuLocalRegion();

    km::RuntimeIsrManager::cpuInit();
    km::InitKernelThread(apic);

    km::SetupApGdt();
    km::XSaveInitApCore();

    km::LocalIsrTable *ist = km::GetLocalIsrTable();
    const km::IsrEntry *spuriousInt = ist->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        KmDebugMessage("[ISR] Spurious interrupt: ", ctx->vector, "\n");
        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();
        return *ctx;
    });
    uint8_t spuriousIdx = ist->index(spuriousInt);
    KmDebugMessage("[SMP] Started AP ", apic->id(), ", Spurious Vector: ", spuriousIdx, "\n");

    apic->setSpuriousVector(spuriousIdx);
    apic->enable();
    apic->eoi();

    km::EnableInterrupts();

    //
    // Copy the callback and user pointer, the header will be unmapped after
    // header->ready is set on the last cpu.
    //
    km::SmpInitCallback callback = header->callback;
    void *user = header->user;

    km::SetupUserMode(header->memory);

    header->ready.test_and_set();

    callback(ist, apic.pointer(), user);
    KM_PANIC("SMP callback returned.");
}

static uintptr_t AllocSmpStack(km::SystemMemory& memory) {
    return (uintptr_t)memory.allocateStack(km::kStartupStackSize).vaddr + km::kStartupStackSize;
}

static SmpInfoHeader SetupSmpInfoHeader(
    km::SystemMemory *memory,
    km::IApic *pic,
    km::SmpInitCallback callback,
    void *user
) {
    km::PageTables& vmm = memory->systemTables();

    km::PhysicalAddress pml4 = vmm.root();
    KM_CHECK(pml4 < UINT32_MAX, "PML4 address is above the 4G range.");

    return SmpInfoHeader {
        .startAddress = (uintptr_t)KmSmpStartup,
        .pat = x64::LoadPatMsr(),
        .pml4 = uint32_t(pml4.address),
        .gdt = km::GetBootGdt(),
        .gdtr = {
            .limit = sizeof(SmpInfoHeader::gdt) - 1,
            .base = offsetof(SmpInfoHeader, gdt) + kSmpInfo.address,
        },
        .bspApic = pic,
        .memory = memory,
        .callback = callback,
        .user = user,
    };
}

size_t km::GetStartupMemorySize(const acpi::AcpiTables &acpiTables) {
    size_t apCount = acpiTables.lapicCount();
    return apCount * kStartupMemorySize;
}

void km::InitSmp(
    km::SystemMemory& memory,
    km::IApic *bsp,
    const acpi::AcpiTables& acpiTables,
    SmpInitCallback callback,
    void *user
) {
    KmDebugMessage("[SMP] Starting APs.\n");
    AddressSpace &systemTables = memory.pageTables();

    //
    // Copy the SMP blob to the correct location.
    //
    size_t blobSize = GetSmpBlobSize();
    MemoryRange smpStartRange = km::PageAligned(MemoryRange::of(kSmpStart, blobSize));
    void *smpStartBlob = memory.map(smpStartRange, km::PageFlags::eAll);
    memcpy(smpStartBlob, _binary_smp_start, blobSize);

    uint32_t bspId = bsp->id();

    KmDebugMessage("[SMP] BSP ID: ", bspId, "\n");

    SmpInfoHeader *smpInfo = memory.mapObject<SmpInfoHeader>(kSmpInfo);
    MemoryRange smpInfoRange = MemoryRange::of(kSmpInfo, x64::kPageSize);

    //
    // Also identity map the SMP blob and info regions, it makes jumping to compatibility mode easier.
    // I think theres a better way to do this, but I'm not sure what it is.
    //
    if (OsStatus status = systemTables.reserve(MappingOf(smpInfoRange, (void*)kSmpInfo.address))) {
        KmDebugMessage("[SMP] Failed to reserve smp info region: ", status, "\n");
        KM_PANIC("Failed to reserve smp info region.");
    }

    if (OsStatus status = systemTables.reserve(MappingOf(smpStartRange, (void*)kSmpStart.address), PageFlags::eAll)) {
        KmDebugMessage("[SMP] Failed to reserve smp blob region: ", status, "\n");
        KM_PANIC("Failed to reserve smp blob region.");
    }

    SmpInfoHeader header = SetupSmpInfoHeader(&memory, bsp, callback, user);
    memcpy(smpInfo, &header, sizeof(header));

    for (const acpi::MadtEntry *madt : *acpiTables.madt()) {
        if (madt->type != acpi::MadtEntryType::eLocalApic)
            continue;

        const acpi::MadtEntry::LocalApic localApic = madt->apic;

        //
        // No need to start the BSP.
        //
        if (localApic.apicId == bspId)
            continue;

        if (!localApic.isEnabled() && !localApic.isOnlineCapable()) {
            KmDebugMessage("[SMP] Skipping APIC ID: ", localApic.apicId, ", core is marked as inoperable.\n");
            continue;
        }

        smpInfo->stack = AllocSmpStack(memory);
        smpInfo->ready.clear();

        KmDebugMessage("[SMP] Starting APIC ID: ", localApic.apicId, "\n");

        //
        // Send the INIT IPI
        //
        bsp->sendIpi(localApic.apicId, apic::IpiAlert::init());

        // TODO: sleep

        KmDebugMessage("[SMP] Sending SIPI to APIC ID: ", localApic.apicId, "\n");

        //
        // Send the start IPI
        //
        bsp->sendIpi(localApic.apicId, apic::IpiAlert::sipi(kSmpStart));

        KmDebugMessage("[SMP] Waiting for APIC ID: ", localApic.apicId, " to start.\n");

        // TODO: should really have a condition variable here
        while (!smpInfo->ready.test()) {
            //
            // Spin until the core is ready
            //
            _mm_pause();
        }
    }

    //
    // Now that we're finished, cleanup the smp blob and startup area.
    //

    //
    // Unmap the smp blob and info regions.
    //
    memory.unmap(smpStartBlob, blobSize);
    memory.unmap(smpInfo, sizeof(SmpInfoHeader));

    //
    // And unmap the identity mappings.
    //
    memory.unmap((void*)kSmpInfo.address, sizeof(SmpInfoHeader));
    memory.unmap((void*)kSmpStart.address, blobSize);

    //
    // Reserve the smp info and blob regions again to prevent them from being
    // reused by the kernel.
    // This is a bit of a hack, but it works.
    //
    memory.reserveVirtual({ std::bit_cast<void*>(kSmpInfo), std::bit_cast<void*>(kSmpInfo + sizeof(SmpInfoHeader)) });
    memory.reserveVirtual({ std::bit_cast<void*>(kSmpStart), std::bit_cast<void*>(kSmpStart + blobSize) });
}
