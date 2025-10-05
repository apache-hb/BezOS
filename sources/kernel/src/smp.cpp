#include "smp.hpp"

#include "isr/isr.hpp"
#include "isr/runtime.hpp"

#include "gdt.hpp"
#include "kernel.hpp"
#include "logger/categories.hpp"
#include "panic.hpp"
#include "pat.hpp"
#include "processor.hpp"
#include "syscall.hpp"
#include "thread.hpp"
#include "xsave.hpp"

#include <atomic>

namespace isr = km::isr;

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

static uintptr_t GetSmpBlobSize() {
    return (uintptr_t)_binary_smp_end - (uintptr_t)_binary_smp_start;
}

static constexpr km::PhysicalAddressEx kSmpInfo = 0x7000;
static constexpr km::PhysicalAddressEx kSmpStart = 0x8000;

extern "C" [[noreturn]] void KmSmpStartup(SmpInfoHeader *header) {
    km::setupInitialGdt();
    km::loadIdt();

    km::Apic apic = km::InitApApic(header->memory->pageTables(), header->bspApic);

    km::InitCpuLocalRegion();

    km::RuntimeIsrManager::cpuInit();
    km::InitKernelThread(apic);

    km::SetupApGdt();
    km::XSaveInitApCore();

    InitLog.dbgf("Started AP ", apic->id(), ", Spurious Vector: ", isr::kSpuriousVector);

    apic->setSpuriousVector(isr::kSpuriousVector);
    apic->enable();
    apic->eoi();

    km::enableInterrupts();

    //
    // Copy the callback and user pointer, the header will be unmapped after
    // header->ready is set on the last cpu.
    //
    km::SmpInitCallback callback = header->callback;
    void *user = header->user;

    km::SetupUserMode(header->memory);

    header->ready.test_and_set();

    callback(apic.pointer(), user);
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

    km::PhysicalAddressEx pml4 = vmm.root();
    KM_CHECK(pml4 < UINT32_MAX, "PML4 address is above the 4G range.");

    return SmpInfoHeader {
        .startAddress = (uintptr_t)KmSmpStartup,
        .pat = x64::loadPatMsr(),
        .pml4 = uint32_t(pml4.address),
        .gdt = km::getBootGdt(),
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
    VmemAllocation smpInfoAlloc;
    VmemAllocation smpStartAlloc;

    InitLog.infof("Starting APs.");
    AddressSpace &addressSpace = memory.pageTables();
    PageTables &kernelTables = *addressSpace.tables();

    size_t blobSize = GetSmpBlobSize();
    MemoryRangeEx smpInfoRange = MemoryRangeEx::of(kSmpInfo, x64::kPageSize);
    MemoryRangeEx smpStartRange = km::alignedOut(MemoryRangeEx::of(kSmpStart, blobSize), x64::kPageSize);

    //
    // Copy the SMP blob to the correct location.
    //
    if (OsStatus status = addressSpace.map(smpStartRange, PageFlags::eAll, MemoryType::eWriteBack, &smpStartAlloc)) {
        InitLog.fatalf("Failed to map smp blob region: ", OsStatusId(status));
        KM_PANIC("Failed to map smp blob region.");
    }

    void *smpStartBlob = std::bit_cast<void*>(smpStartAlloc.address());
    memcpy(smpStartBlob, _binary_smp_start, blobSize);

    uint32_t bspId = bsp->id();

    InitLog.dbgf("BSP ID: ", bspId);

    SmpInfoHeader *smpInfo = addressSpace.mapObject<SmpInfoHeader>(kSmpInfo, PageFlags::eData, MemoryType::eWriteBack, &smpInfoAlloc);

    //
    // Also identity map the SMP blob and info regions, it makes jumping to compatibility mode easier.
    // I think theres a better way to do this, but I'm not sure what it is.
    //
    AddressMapping smpInfoIdentity = MappingOf(smpInfoRange, (void*)kSmpInfo.address);
    AddressMapping smpStartIdentity = MappingOf(smpStartRange, (void*)kSmpStart.address);

    if (OsStatus status = kernelTables.map(smpInfoIdentity, PageFlags::eData, MemoryType::eWriteBack)) {
        InitLog.fatalf("Failed to reserve identity mapping for smp info region: ", OsStatusId(status));
        KM_PANIC("Failed to reserve smp info region.");
    }

    if (OsStatus status = kernelTables.map(smpStartIdentity, PageFlags::eCode, MemoryType::eWriteBack)) {
        InitLog.fatalf("Failed to reserve identity mapping for  smp blob region: ", OsStatusId(status));
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
            InitLog.warnf("Skipping APIC ID: ", localApic.apicId, ", core is marked as inoperable.");
            continue;
        }

        smpInfo->stack = AllocSmpStack(memory);
        smpInfo->ready.clear();

        //
        // Send the INIT IPI
        //
        bsp->sendIpi(localApic.apicId, apic::IpiAlert::init());

        // TODO: sleep

        //
        // Send the start IPI
        //
        bsp->sendIpi(localApic.apicId, apic::IpiAlert::sipi(kSmpStart));

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
    if (OsStatus status = addressSpace.unmap(smpInfoAlloc)) {
        InitLog.fatalf("Failed to unmap smp info region: ", OsStatusId(status));
        KM_PANIC("Failed to unmap smp info region.");
    }

    if (OsStatus status = addressSpace.unmap(smpStartAlloc)) {
        InitLog.fatalf("Failed to unmap smp blob region: ", OsStatusId(status));
        KM_PANIC("Failed to unmap smp blob region.");
    }

    //
    // And unmap the identity mappings.
    //
    if (OsStatus status = kernelTables.unmap(smpInfoIdentity.virtualRange())) {
        InitLog.fatalf("Failed to unmap smp info region: ", OsStatusId(status));
        KM_PANIC("Failed to unmap smp info region.");
    }

    if (OsStatus status = kernelTables.unmap(smpStartIdentity.virtualRange())) {
        InitLog.fatalf("Failed to unmap smp blob region: ", OsStatusId(status));
        KM_PANIC("Failed to unmap smp blob region.");
    }
}
