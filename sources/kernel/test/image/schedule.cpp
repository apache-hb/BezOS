// SPDX-License-Identifier: GPL-3.0-only

#include <numeric>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "allocator/synchronized.hpp"
#include "apic.hpp"

#include "arch/cr0.hpp"
#include "arch/cr4.hpp"

#include "acpi/acpi.hpp"

#include <bezos/subsystem/hid.h>
#include <bezos/facility/device.h>
#include <bezos/subsystem/ddi.h>
#include <bezos/facility/vmem.h>

#include "setup.hpp"
#include "delay.hpp"
#include "devices/ddi.hpp"
#include "devices/hid.hpp"
#include "display.hpp"
#include "drivers/block/ramblk.hpp"
#include "elf.hpp"
#include "fs2/vfs.hpp"
#include "gdt.hpp"
#include "hid/hid.hpp"
#include "hid/ps2.hpp"
#include "hypervisor.hpp"
#include "isr/isr.hpp"
#include "isr/runtime.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "memory/memory.hpp"
#include "notify.hpp"
#include "panic.hpp"
#include "processor.hpp"
#include "process/schedule.hpp"
#include "smp.hpp"
#include "std/spinlock.hpp"
#include "std/static_vector.hpp"
#include "syscall.hpp"
#include "thread.hpp"
#include "uart.hpp"
#include "user/user.hpp"

#include "memory/layout.hpp"

#include "allocator/tlsf.hpp"

#include "arch/intrin.hpp"

#include "kernel.hpp"

#include "util/memory.hpp"

#include "fs2/vfs.hpp"
#include "fs2/tarfs.hpp"
#include "fs2/ramfs.hpp"

using namespace km;
using namespace stdx::literals;

static constexpr bool kUseX2Apic = true;
static constexpr bool kSelfTestIdt = true;
static constexpr bool kSelfTestApic = true;
static constexpr bool kEnableSmp = true;

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;

class SerialLog final : public IOutStream {
    SerialPort mPort;

public:
    SerialLog(SerialPort port)
        : mPort(port)
    { }

    constexpr SerialLog() { }

    void write(stdx::StringView message) override {
        mPort.print(message);
    }
};

template<typename T>
class TerminalLog final : public IOutStream {
    T mTerminal;

public:
    constexpr TerminalLog()
        : mTerminal(T(sm::noinit{}))
    { }

    constexpr TerminalLog(T terminal)
        : mTerminal(terminal)
    { }

    void write(stdx::StringView message) override {
        mTerminal.print(message);
    }

    T& get() { return mTerminal; }
};

// load bearing constinit, clang has a bug in c++26 mode
// where it doesnt emit a warning for global constructors in all cases.
constinit static SerialLog gSerialLog;
constinit static TerminalLog<DirectTerminal> gDirectTerminalLog;

constinit static stdx::StaticVector<IOutStream*, 4> gLogTargets;

class DebugLog final : public IOutStream {
public:
    constexpr DebugLog() { }

    void write(stdx::StringView message) override {
        for (IOutStream *target : gLogTargets) {
            target->write(message);
        }
    }
};

constinit static DebugLog gDebugLog;

km::IOutStream *km::GetDebugStream() {
    return &gDebugLog;
}

constinit km::CpuLocal<SystemGdt> km::tlsSystemGdt;
constinit km::CpuLocal<x64::TaskStateSegment> km::tlsTaskState;

static constinit x64::TaskStateSegment gBootTss{};
static constinit SystemGdt gBootGdt{};

static constexpr size_t kTssStackSize = x64::kPageSize * 4;

enum {
    kIstTrap = 1,
    kIstTimer = 2,
    kIstNmi = 3,
};

void km::SetupInitialGdt(void) {
    InitGdt(gBootGdt.entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
}

void km::SetupApGdt(void) {
    // Save the gs/fs/kgsbase values, as setting the GDT will clear them
    CpuLocalRegisters tls = LoadTlsRegisters();

    tlsTaskState = x64::TaskStateSegment {
        .ist1 = (uintptr_t)aligned_alloc(x64::kPageSize, kTssStackSize) + kTssStackSize,
        .ist2 = (uintptr_t)aligned_alloc(x64::kPageSize, kTssStackSize) + kTssStackSize,
        .ist3 = (uintptr_t)aligned_alloc(x64::kPageSize, kTssStackSize) + kTssStackSize,
        .iopbOffset = sizeof(x64::TaskStateSegment),
    };

    tlsSystemGdt = km::GetSystemGdt(&tlsTaskState);
    InitGdt(tlsSystemGdt->entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
    __ltr(SystemGdt::eTaskState0 * 0x8);

    StoreTlsRegisters(tls);
}

static constexpr size_t kStage1AllocMinSize = sm::megabytes(1).bytes();
static constexpr size_t kCommittedRegionSize = sm::megabytes(16).bytes();

struct KernelLayout {
    /// @brief Virtual address space reserved for the kernel.
    /// Space for dynamic allocations, kernel heap, kernel stacks, etc.
    VirtualRange system;

    /// @brief Non-pageable memory.
    AddressMapping committed;

    /// @brief All framebuffers.
    VirtualRange framebuffers;

    /// @brief Statically allocated memory for the kernel.
    /// Includes text, data, bss, and other sections.
    AddressMapping kernel;

    void *getFrameBuffer(std::span<const boot::FrameBuffer> fbs, size_t index) const {
        size_t offset = 0;
        for (size_t i = 0; i < index; i++) {
            offset += fbs[i].size();
        }

        return (void*)((uintptr_t)framebuffers.front + offset);
    }

    uintptr_t committedSlide() const {
        return committed.slide();
    }
};

extern "C" {
    extern char __kernel_start[];
    extern char __kernel_end[];
}

static size_t GetKernelSize() {
    return (uintptr_t)__kernel_end - (uintptr_t)__kernel_start;
}

/// @brief Factory to reserve sections of virtual address space.
///
/// Grows down from the top of virtual address space.
class AddressSpaceBuilder {
    uintptr_t mOffset = 0;

public:
    AddressSpaceBuilder(uintptr_t top)
        : mOffset(top)
    { }

    /// @brief Reserve a section of virtual address space.
    ///
    /// The reserved memory is aligned to the smallest page size.
    ///
    /// @param memory The size of the memory to reserve.
    /// @return The reserved virtual address range.
    VirtualRange reserve(size_t memory) {
        uintptr_t front = mOffset;
        mOffset += sm::roundup(memory, x64::kPageSize);

        return VirtualRange { (void*)front, (void*)mOffset };
    }
};

static size_t TotalDisplaySize(std::span<const boot::FrameBuffer> displays) {
    return std::accumulate(displays.begin(), displays.end(), 0zu, [](size_t sum, const boot::FrameBuffer& framebuffer) {
        return sum + framebuffer.size();
    });
}

struct MemoryMap {
    mem::TlsfAllocator allocator;
    stdx::Vector3<boot::MemoryRegion> memmap;

    MemoryMap(mem::TlsfAllocator alloc)
        : allocator(std::move(alloc))
        , memmap(&allocator)
    { }

    void sortMemoryMap() {
        std::sort(memmap.begin(), memmap.end(), [](const boot::MemoryRegion& a, const boot::MemoryRegion& b) {
            return a.range.front < b.range.front;
        });
    }

    /// @brief Find and reserve a section of physical memory.
    ///
    /// @param size The size of the memory to reserve.
    /// @return The reserved physical memory range.
    km::MemoryRange reserve(size_t size) {
        for (boot::MemoryRegion& entry : memmap) {
            if (!entry.isUsable())
                continue;

            if (entry.size() < size)
                continue;

            if (IsLowMemory(entry.range))
                continue;

            km::MemoryRange range = { entry.range.front, entry.range.front + size };
            entry.range = entry.range.cut(range);

            memmap.add({ boot::MemoryRegion::eKernelRuntimeData, range });

            sortMemoryMap();

            return range;
        }

        return km::MemoryRange{};
    }

    void insert(boot::MemoryRegion memory) {
        for (size_t i = 0; i < memmap.count(); i++) {
            boot::MemoryRegion& entry = memmap[i];
            if (entry.range.intersects(memory.range)) {
                entry.range = entry.range.cut(memory.range);
            }
        }

        memmap.add(memory);

        sortMemoryMap();
    }
};

static KernelLayout BuildKernelLayout(
    uintptr_t vaddrbits,
    km::PhysicalAddress kernelPhysicalBase,
    const void *kernelVirtualBase,
    MemoryMap& memory,
    std::span<const boot::FrameBuffer> displays
) {
    size_t framebuffersSize = TotalDisplaySize(displays);

    // reserve the top 1/4 of the address space for the kernel
    uintptr_t dataFront = -(1ull << (vaddrbits - 1));
    uintptr_t dataBack = -(1ull << (vaddrbits - 2));

    VirtualRange data = { (void*)dataFront, (void*)dataBack };

    AddressSpaceBuilder builder { dataBack };

    km::MemoryRange committedPhysicalMemory = memory.reserve(kCommittedRegionSize);
    if (committedPhysicalMemory.isEmpty()) {
        WriteMemoryMap(memory.memmap);
        KmDebugMessage("[INIT] Failed to reserve memory for committed region ", sm::bytes(kCommittedRegionSize), ".\n");
        KM_PANIC("Failed to reserve memory for committed region.");
    }

    VirtualRange committedVirtualRange = builder.reserve(kCommittedRegionSize);
    km::AddressMapping committed = { committedVirtualRange.front, committedPhysicalMemory.front, kCommittedRegionSize };

    VirtualRange framebuffers = builder.reserve(framebuffersSize);

    AddressMapping kernel = { kernelVirtualBase, kernelPhysicalBase, GetKernelSize() };

    KmDebugMessage("[INIT] Kernel layout:\n");
    KmDebugMessage("[INIT] Data         : ", data, "\n");
    KmDebugMessage("[INIT] Committed    : ", committed, "\n");
    KmDebugMessage("[INIT] Framebuffers : ", framebuffers, "\n");
    KmDebugMessage("[INIT] Kernel       : ", kernel, "\n");

    KM_CHECK(data.isValid(), "Invalid data range.");
    KM_CHECK(committedVirtualRange.isValid(), "Invalid committed range.");
    KM_CHECK(framebuffers.isValid(), "Invalid framebuffers range.");
    KM_CHECK(kernel.physicalRange().isValid(), "Invalid kernel range.");

    KM_CHECK(!data.intersects(committedVirtualRange), "Committed range overlaps with data range.");
    KM_CHECK(!data.intersects(framebuffers), "Framebuffers overlap with data range.");

    KernelLayout layout = {
        .system = data,
        .committed = committed,
        .framebuffers = framebuffers,
        .kernel = kernel,
    };

    return layout;
}

static size_t GetEarlyMemorySize(std::span<const boot::MemoryRegion> memmap) {
    size_t size = 0;
    for (const boot::MemoryRegion& entry : memmap) {
        if (!entry.isUsable() && !entry.isReclaimable())
            continue;

        if (IsLowMemory(entry.range))
            continue;

        size += entry.size();
    }

    // should only need around 1% of total memory for the early allocator
    return sm::roundup(size / 100, x64::kPageSize);
}

static boot::MemoryRegion FindEarlyAllocatorRegion(std::span<const boot::MemoryRegion> memmap, size_t size) {
    for (const boot::MemoryRegion& entry : memmap) {
        if (!entry.isUsable())
            continue;

        if (IsLowMemory(entry.range))
            continue;

        if (entry.range.size() < size)
            continue;

        return entry;
    }

    WriteMemoryMap(memmap);
    KM_PANIC("No suitable memory region found for early allocator.");
}

static MemoryMap *AllocateEarlyMemory(void *vaddr, size_t size) {
    mem::TlsfAllocator allocator { vaddr, size };

    void *mem = allocator.allocateAligned(sizeof(MemoryMap), alignof(MemoryMap));

    return new (mem) MemoryMap(std::move(allocator));
}

static std::tuple<MemoryMap*, km::AddressMapping> CreateEarlyAllocator(std::span<const boot::MemoryRegion> memmap, uintptr_t hhdmOffset) {
    size_t earlyAllocSize = std::max(GetEarlyMemorySize(memmap), kStage1AllocMinSize);

    boot::MemoryRegion region = FindEarlyAllocatorRegion(memmap, earlyAllocSize);
    km::MemoryRange range = { region.range.front, region.range.front + earlyAllocSize };

    void *vaddr = std::bit_cast<void*>(region.range.front + hhdmOffset);
    MemoryMap *memory = AllocateEarlyMemory(vaddr, earlyAllocSize);
    std::copy(memmap.begin(), memmap.end(), std::back_inserter(memory->memmap));

    memory->insert({ boot::MemoryRegion::eKernelRuntimeData, range });

    return std::make_tuple(memory, km::AddressMapping { vaddr, range.front, earlyAllocSize });
}

static void MapDisplayRegions(PageTables& vmm, std::span<const boot::FrameBuffer> framebuffers, VirtualRange range) {
    uintptr_t framebufferBase = (uintptr_t)range.front;
    for (const boot::FrameBuffer& framebuffer : framebuffers) {
        // remap the framebuffer into its final location
        km::AddressMapping fb = { (void*)framebufferBase, framebuffer.paddr, framebuffer.size() };
        vmm.map(fb, PageFlags::eData, MemoryType::eWriteCombine);

        framebufferBase += framebuffer.size();
    }
}

static void MapKernelRegions(PageTables &vmm, const KernelLayout& layout) {
    auto [kernelVirtualBase, kernelPhysicalBase, _] = layout.kernel;
    KmMapKernel(vmm, kernelPhysicalBase, kernelVirtualBase);
}

static void MapDataRegion(PageTables &vmm, km::AddressMapping mapping) {
    vmm.map(mapping, PageFlags::eData);
}

/// Everything in this structure is owned by the stage1 memory allocator.
/// Once that allocator is destroyed this memory is no longer mapped.
struct Stage1MemoryInfo {
    MemoryMap *earlyMemory;
    KernelLayout layout;
    std::span<boot::FrameBuffer> framebuffers;
};

static boot::FrameBuffer *CloneFrameBuffers(mem::IAllocator *alloc, std::span<const boot::FrameBuffer> framebuffers) {
    boot::FrameBuffer *fbs = alloc->allocateArray<boot::FrameBuffer>(framebuffers.size());
    std::copy(framebuffers.begin(), framebuffers.end(), fbs);
    return fbs;
}

static Stage1MemoryInfo InitStage1Memory(const boot::LaunchInfo& launch, const km::ProcessorInfo& processor) {
    PageMemoryTypeLayout pat = SetupPat();

    km::AddressMapping stack = launch.stack;
    PageBuilder pm = PageBuilder { processor.maxpaddr, processor.maxvaddr, launch.hhdmOffset, pat };

    auto [earlyMemory, earlyRegion] = CreateEarlyAllocator(launch.memmap, launch.hhdmOffset);

    KernelLayout layout = BuildKernelLayout(
        processor.maxvaddr,
        launch.kernelPhysicalBase,
        launch.kernelVirtualBase,
        *earlyMemory,
        launch.framebuffers
    );

    mem::IAllocator *alloc = &earlyMemory->allocator;

    boot::FrameBuffer *framebuffers = CloneFrameBuffers(alloc, launch.framebuffers);

    PageTables vmm(&pm, &earlyMemory->allocator);

    // initialize our own page tables and remap everything into it
    MapKernelRegions(vmm, layout);

    // move our stack out of reclaimable memory
    MapDataRegion(vmm, stack);

    // map the early allocator region
    MapDataRegion(vmm, earlyRegion);

    // map the non-pageable memory
    MapDataRegion(vmm, layout.committed);

    // remap framebuffers

    MapDisplayRegions(vmm, launch.framebuffers, layout.framebuffers);

    // once it is safe to remap the boot memory, do so
    KmUpdateRootPageTable(pm, vmm);

    // can't log anything here as we need to move the framebuffer first

    // Now update the terminal to use the new memory layout
    if (!launch.framebuffers.empty()) {
        gDirectTerminalLog.get()
            .display()
            .setAddress(layout.getFrameBuffer(launch.framebuffers, 0));
    }

    return Stage1MemoryInfo { earlyMemory, layout, std::span(framebuffers, launch.framebuffers.size()) };
}

struct Stage2MemoryInfo {
    SystemMemory *memory;
    KernelLayout layout;
};

using SynchronizedTlsfAllocator = mem::SynchronizedAllocator<mem::TlsfAllocator>;

static Stage2MemoryInfo *InitStage2Memory(
    const boot::LaunchInfo& launch,
    const km::ProcessorInfo& processor,
    const Stage1MemoryInfo& stage1
) {
    km::AddressMapping stack = launch.stack;
    KernelLayout layout = stage1.layout;
    std::span<boot::FrameBuffer> framebuffers = stage1.framebuffers;
    MemoryMap *earlyMemory = stage1.earlyMemory;

    PageBuilder pm = PageBuilder { processor.maxpaddr, processor.maxvaddr, layout.committedSlide(), GetDefaultPatLayout() };

    InitGlobalAllocator((void*)layout.committed.vaddr, layout.committed.size);

    Stage2MemoryInfo *stage2 = new Stage2MemoryInfo();

    static constexpr size_t kPtAllocSize = sm::megabytes(1).bytes();
    auto *ptAllocator = new SynchronizedTlsfAllocator(aligned_alloc(x64::kPageSize, kPtAllocSize), kPtAllocSize);

    SystemMemory *memory = new SystemMemory(earlyMemory->memmap, stage1.layout.system, DefaultUserArea(), pm, ptAllocator);
    KM_CHECK(memory != nullptr, "Failed to allocate memory for SystemMemory.");

    stage2->memory = memory;
    stage2->layout = layout;

    // carry forward the mappings made in stage 1
    // that are still required for kernel runtime
    memory->pmm.markUsed(stack.physicalRange());
    memory->pmm.markUsed(layout.committed.physicalRange());
    memory->pmm.markUsed(layout.kernel.physicalRange());
    memory->pmm.markUsed({ 0zu, kLowMemory });

    memory->vmm.markUsed(stack.virtualRange());
    memory->vmm.markUsed(layout.committed.virtualRange());
    memory->vmm.markUsed(layout.kernel.virtualRange());

    // initialize our own page tables and remap everything into it
    MapKernelRegions(memory->pt, layout);

    // map the stack again
    MapDataRegion(memory->pt, stack);

    // carry forward the non-pageable memory
    MapDataRegion(memory->pt, layout.committed);

    // remap framebuffers
    MapDisplayRegions(memory->pt, framebuffers, layout.framebuffers);

    // once it is safe to remap the boot memory, do so
    KmUpdateRootPageTable(memory->pager, memory->pt);

    return stage2;
}

static km::IsrContext SpuriousVector(km::IsrContext *ctx) {
    KmDebugMessage("[ISR] Spurious interrupt: ", ctx->vector, "\n");
    km::IApic *pic = km::GetCpuLocalApic();
    pic->eoi();
    return *ctx;
}

struct ApicInfo {
    km::Apic apic;
    uint8_t spuriousInt;
};

static ApicInfo EnableBootApic(km::SystemMemory& memory, bool useX2Apic) {
    km::Apic pic = InitBspApic(memory, useX2Apic);

    // setup tls now that we have the lapic id

    km::InitCpuLocalRegion();
    km::RuntimeIsrManager::cpuInit();

    km::EnableCpuLocalIsrTable();

    SetDebugLogLock(DebugLogLockType::eSpinLock);
    km::InitKernelThread(pic);

    km::LocalIsrTable *ist = km::GetLocalIsrTable();

    const IsrEntry *spuriousInt = ist->allocate(SpuriousVector);
    uint8_t spuriousIdx = ist->index(spuriousInt);
    KmDebugMessage("[INIT] APIC ID: ", pic->id(), ", Version: ", pic->version(), ", Spurious vector: ", spuriousIdx, "\n");

    pic->setSpuriousVector(spuriousIdx);
    pic->enable();

    if (kSelfTestApic) {
        IsrCallback testIsr = [](km::IsrContext *ctx) -> km::IsrContext {
            KmDebugMessage("[TEST] Handled isr: ", ctx->vector, "\n");
            km::IApic *pic = km::GetCpuLocalApic();
            pic->eoi();
            return *ctx;
        };

        const IsrEntry *testInt = ist->allocate(testIsr);
        uint8_t testIdx = ist->index(testInt);

        KmDebugMessage("[TEST] Testing self-IPI ", testIdx, "\n");

        //
        // Test that both self-IPI methods work.
        //
        pic->sendIpi(apic::IcrDeliver::eSelf, testIdx);
        pic->selfIpi(testIdx);

        ist->release(testInt, testIsr);
    }

    return ApicInfo { pic, spuriousIdx };
}

static void InitBootTerminal(std::span<const boot::FrameBuffer> framebuffers) {
    if (framebuffers.empty()) return;

    boot::FrameBuffer framebuffer = framebuffers.front();
    km::Canvas display { framebuffer, (uint8_t*)(framebuffer.vaddr) };
    gDirectTerminalLog = DirectTerminal(display);
    gLogTargets.add(&gDirectTerminalLog);
}

static SerialPortStatus InitSerialPort(ComPortInfo info) {
    if (OpenSerialResult com1 = OpenSerial(info)) {
        return com1.status;
    } else {
        gSerialLog = SerialLog(com1.port);
        gLogTargets.add(&gSerialLog);
        return SerialPortStatus::eOk;
    }
}

static void InitPortDelay() {
    KmSetPortDelayMethod(x64::PortDelay::ePostCode);
}

static void SetupInterruptStacks(uint16_t cs) {
    gBootTss = x64::TaskStateSegment{
        .ist1 = (uintptr_t)aligned_alloc(kTssStackSize, x64::kPageSize) + kTssStackSize,
        .ist2 = (uintptr_t)aligned_alloc(kTssStackSize, x64::kPageSize) + kTssStackSize,
        .ist3 = (uintptr_t)aligned_alloc(kTssStackSize, x64::kPageSize) + kTssStackSize,
    };

    gBootGdt.setTss(&gBootTss);
    __ltr(SystemGdt::eTaskState0 * 0x8);

    for (uint8_t i = 0; i < isr::kExceptionCount; i++) {
        UpdateIdtEntry(i, cs, x64::Privilege::eSupervisor, kIstTrap);
    }

    UpdateIdtEntry(isr::NMI, cs, x64::Privilege::eSupervisor, kIstNmi);
    UpdateIdtEntry(isr::MCE, cs, x64::Privilege::eSupervisor, kIstNmi);
}

static void InitStage1Idt(uint16_t cs) {
    InitInterrupts(cs);
    InstallExceptionHandlers(GetSharedIsrTable());
    SetupInterruptStacks(cs);

    if (kSelfTestIdt) {
        km::LocalIsrTable *ist = GetLocalIsrTable();
        IsrCallback old = ist->install(64, [](km::IsrContext *context) -> km::IsrContext {
            KmDebugMessage("[TEST] Handled isr: ", context->vector, "\n");
            return *context;
        });

        __int<64>();

        ist->install(64, old);
    }
}

static void NormalizeProcessorState() {
    x64::Cr0 cr0 = x64::Cr0::of(x64::Cr0::PG | x64::Cr0::WP | x64::Cr0::NE | x64::Cr0::ET | x64::Cr0::PE);
    x64::Cr0::store(cr0);

    x64::Cr4 cr4 = x64::Cr4::of(x64::Cr4::PAE);
    x64::Cr4::store(cr4);

    // Enable NXE bit
    kEfer.store(kEfer.load() | (1 << 11));

    kGsBase.store(0);
}

static vfs2::VfsRoot *gVfsRoot = nullptr;
static SystemObjects *gSystemObjects = nullptr;
static NotificationStream *gNotificationStream = nullptr;
static Scheduler *gScheduler = nullptr;
static SystemMemory *gMemory = nullptr;

Scheduler *km::GetScheduler() {
    return gScheduler;
}

SystemMemory *km::GetSystemMemory() {
    return gMemory;
}

static void CreateNotificationQueue() {
    static constexpr size_t kAqMemorySize = sm::kilobytes(128).bytes();
    void *memory = aligned_alloc(x64::kPageSize, kAqMemorySize);
    InitAqAllocator(memory, kAqMemorySize);

    gNotificationStream = new NotificationStream();
}

static const void *TranslateUserPointer(const void *userAddress) {
    PageTables& pt = gMemory->pt;
    if (bool(pt.getMemoryFlags(userAddress) & (PageFlags::eUser | PageFlags::eRead))) {
        return userAddress;
    }

    return nullptr;
}

static void MountRootVfs() {
    KmDebugMessage("[VFS] Initializing VFS.\n");
    gVfsRoot = new vfs2::VfsRoot();

    {
        vfs2::IVfsNode *node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(vfs2::BuildPath("System", "Config"), &node)) {
            KmDebugMessage("[VFS] Failed to create path: ", status, "\n");
        }
    }

    {
        vfs2::IVfsNode *node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(vfs2::BuildPath("Users", "Admin"), &node)) {
            KmDebugMessage("[VFS] Failed to create path: ", status, "\n");
        }

        if (OsStatus status = gVfsRoot->mkpath(vfs2::BuildPath("Users", "Guest"), &node)) {
            KmDebugMessage("[VFS] Failed to create path: ", status, "\n");
        }
    }

    {
        vfs2::IVfsNode *node = nullptr;
        if (OsStatus status = gVfsRoot->create(vfs2::BuildPath("Users", "Guest", "motd.txt"), &node)) {
            KmDebugMessage("[VFS] Failed to create path: ", status, "\n");
        }

        std::unique_ptr<vfs2::IVfsNodeHandle> motd;
        if (OsStatus status = node->open(std::out_ptr(motd))) {
            KmDebugMessage("[VFS] Failed to open file: ", status, "\n");
        }

        char data[] = "Welcome.\n";
        vfs2::WriteRequest request {
            .begin = std::begin(data),
            .end = std::end(data),
        };
        vfs2::WriteResult result;

        if (OsStatus status = motd->write(request, &result)) {
            KmDebugMessage("[VFS] Failed to write file: ", status, "\n");
        }
    }
}

static void MountInitArchive(MemoryRange initrd, SystemMemory& memory) {
    KmDebugMessage("[INIT] Mounting '/Init'\n");

    //
    // If the initrd is empty then it wasn't found in the boot parameters.
    // Not much we can do without an initrd.
    //
    if (initrd.isEmpty()) {
        KmDebugMessage("[INIT] No initrd found.\n");
        KM_PANIC("No initrd found.");
    }

    void *initrdMemory = memory.map(initrd, PageFlags::eRead);
    sm::SharedPtr<MemoryBlk> block = new MemoryBlk{(std::byte*)initrdMemory, initrd.size()};

    vfs2::IVfsMount *mount = nullptr;
    if (OsStatus status = gVfsRoot->addMountWithParams(&vfs2::TarFs::instance(), vfs2::BuildPath("Init"), &mount, block)) {
        KmDebugMessage("[VFS] Failed to mount initrd: ", status, "\n");
        KM_PANIC("Failed to mount initrd.");
    }
}

static void MountVolatileFolder() {
    KmDebugMessage("[INIT] Mounting '/Volatile'\n");

    vfs2::IVfsMount *mount = nullptr;
    if (OsStatus status = gVfsRoot->addMount(&vfs2::RamFs::instance(), vfs2::BuildPath("Volatile"), &mount)) {
        KmDebugMessage("[VFS] Failed to mount '/Volatile' ", status, "\n");
        KM_PANIC("Failed to mount volatile folder.");
    }
}

static void AddDebugSystemCalls() {
    AddSystemCall(eOsCallDebugLog, [](uint64_t arg0, uint64_t arg1, uint64_t, uint64_t) -> OsCallResult {
        const char *front = (const char*)TranslateUserPointer((const void*)arg0);
        const char *back = (const char*)TranslateUserPointer((const void*)arg1);

        if (front == nullptr || back == nullptr) {
            return CallError(OsStatusInvalidInput);
        }

        stdx::StringView message(front, back);

        KmDebugMessage(message);

        return CallOk(0zu);
    });
}

static void AddVfsSystemCalls() {
    AddSystemCall(eOsCallFileOpen, [](uint64_t userArgPathBegin, uint64_t userArgPathEnd, [[maybe_unused]] uint64_t mode, uint64_t) -> OsCallResult {
        vfs2::VfsString userPath;
        if (OsStatus status = km::CopyUserRange((void*)userArgPathBegin, (void*)userArgPathEnd, &userPath, kMaxPathSize)) {
            return CallError(status);
        }

        if (!vfs2::VerifyPathText(userPath)) {
            return CallError(OsStatusInvalidInput);
        }

        vfs2::VfsPath path{std::move(userPath)};
        vfs2::IVfsNodeHandle *node = nullptr;
        if (OsStatus status = gVfsRoot->open(path, &node)) {
            return CallError(status);
        }

        return CallOk(node);
    });

    AddSystemCall(eOsCallFileRead, [](uint64_t userNodeId, uint64_t userBufferBegin, uint64_t userBufferEnd, uint64_t) -> OsCallResult {
        const void *bufferBegin = TranslateUserPointer((const void*)userBufferBegin);
        const void *bufferEnd = TranslateUserPointer((const void*)userBufferEnd);
        if (bufferBegin == nullptr || bufferEnd == nullptr) {
            return CallError(OsStatusInvalidInput);
        }

        vfs2::IVfsNodeHandle *node = (vfs2::IVfsNodeHandle*)userNodeId;
        vfs2::ReadRequest request {
            .begin = (void*)bufferBegin,
            .end = (void*)bufferEnd,
        };
        vfs2::ReadResult result{};
        if (OsStatus status = node->read(request, &result)) {
            return CallError(status);
        }

        return CallOk(result.read);
    });
}

static void AddDeviceSystemCalls() {
    AddSystemCall(eOsCallDeviceOpen, [](uint64_t userCreateInfo, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        OsDeviceCreateInfo createInfo{};
        if (OsStatus status = km::CopyUserMemory(userCreateInfo, sizeof(createInfo), &createInfo)) {
            return CallError(status);
        }

        vfs2::VfsString userPath;
        if (OsStatus status = km::CopyUserRange(createInfo.NameFront, createInfo.NameBack, &userPath, kMaxPathSize)) {
            return CallError(status);
        }

        if (!vfs2::VerifyPathText(userPath)) {
            return CallError(OsStatusInvalidInput);
        }

        vfs2::VfsPath path{std::move(userPath)};

        sm::uuid uuid = createInfo.InterfaceGuid;

        vfs2::IVfsNodeHandle *handle = nullptr;
        if (OsStatus status = gVfsRoot->device(path, uuid, &handle)) {
            return CallError(status);
        }

        return CallOk(handle);
    });

    AddSystemCall(eOsCallDeviceRead, [](uint64_t userHandle, uint64_t userRequest, uint64_t, uint64_t) -> OsCallResult {
        vfs2::IVfsNodeHandle *handle = (vfs2::IVfsNodeHandle*)userHandle;

        OsDeviceReadRequest request{};
        if (OsStatus status = km::CopyUserMemory(userRequest, sizeof(request), &request)) {
            return CallError(status);
        }

        if (!IsRangeMapped(gMemory->pt, request.BufferFront, request.BufferBack, PageFlags::eUser | PageFlags::eWrite)) {
            return CallError(OsStatusInvalidInput);
        }

        vfs2::ReadRequest readRequest {
            .begin = request.BufferFront,
            .end = request.BufferBack,
            .timeout = request.Timeout,
        };
        vfs2::ReadResult result{};
        if (OsStatus status = handle->read(readRequest, &result)) {
            return CallError(status);
        }

        return CallOk(result.read);
    });

    AddSystemCall(eOsCallDeviceCall, [](uint64_t userHandle, uint64_t userFunction, uint64_t userData, uint64_t) -> OsCallResult {
        vfs2::IVfsNodeHandle *handle = (vfs2::IVfsNodeHandle*)userHandle;

        if (OsStatus status = handle->call(userFunction, (void*)userData)) {
            return CallError(status);
        }

        return CallOk(0zu);
    });
}

static void AddThreadSystemCalls() {
    AddSystemCall(eOsCallThreadSleep, [](uint64_t, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        km::YieldCurrentThread();
        return CallOk(0zu);
    });

    AddSystemCall(eOsCallThreadCreate, [](uint64_t userCreateInfo, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        OsThreadCreateInfo createInfo{};
        if (OsStatus status = km::CopyUserMemory(userCreateInfo, sizeof(createInfo), &createInfo)) {
            return CallError(status);
        }

        if ((createInfo.StackSize % x64::kPageSize != 0) || (createInfo.StackSize < x64::kPageSize)) {
            return CallError(OsStatusInvalidInput);
        }

        stdx::String name;
        if (OsStatus status = km::CopyUserRange(createInfo.NameFront, createInfo.NameBack, &name, kMaxPathSize)) {
            return CallError(status);
        }

        Process * process;
        if (createInfo.Process != OS_HANDLE_INVALID) {
            process = gSystemObjects->getProcess(ProcessId(createInfo.Process & 0x00FF'FFFF'FFFF'FFFF));
        } else {
            process = GetCurrentProcess();
        }

        if (process == nullptr) {
            return CallError(OsStatusNotFound);
        }

        stdx::String stackName = name + " STACK";
        Thread * thread = gSystemObjects->createThread(std::move(name), process);

        PageFlags flags = PageFlags::eUser | PageFlags::eData;
        MemoryType type = MemoryType::eWriteBack;
        km::AddressMapping mapping = gMemory->userAllocate(createInfo.StackSize, flags, type);
        AddressSpace * stackSpace = gSystemObjects->createAddressSpace(std::move(stackName), mapping, flags, type, process);
        thread->stack = stackSpace;
        thread->state = km::IsrContext {
            .rbp = (uintptr_t)(thread->stack.get() + createInfo.StackSize),
            .rip = (uintptr_t)createInfo.EntryPoint,
            .cs = SystemGdt::eLongModeUserCode * 0x8,
            .rflags = 0x202,
            .rsp = (uintptr_t)(thread->stack.get() + createInfo.StackSize),
            .ss = SystemGdt::eLongModeUserData * 0x8,
        };
        return CallError(OsStatusInvalidFunction);
    });

    AddSystemCall(eOsCallThreadDestroy, [](uint64_t, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        return CallError(OsStatusInvalidFunction);
    });

    AddSystemCall(eOsCallThreadCurrent, [](uint64_t, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        return CallError(OsStatusInvalidFunction);
    });

    AddSystemCall(eOsCallThreadStat, [](uint64_t, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        return CallError(OsStatusInvalidFunction);
    });
}

static OsStatus GetMemoryAccess(OsMemoryAccess access, PageFlags *result) {
    PageFlags flags = PageFlags::eUser;
    if (access & eOsMemoryRead) {
        flags |= PageFlags::eRead;
        access &= ~eOsMemoryRead;
    }

    if (access & eOsMemoryWrite) {
        flags |= PageFlags::eWrite;
        access &= ~eOsMemoryWrite;
    }

    if (access & eOsMemoryExecute) {
        flags |= PageFlags::eExecute;
        access &= ~eOsMemoryExecute;
    }

    if (access != 0) {
        return OsStatusInvalidInput;
    }

    *result = flags;
    return OsStatusSuccess;
}

static OsMemoryAccess MakeMemoryAccess(PageFlags flags) {
    OsMemoryAccess access = 0;
    if (bool(flags & PageFlags::eRead)) {
        access |= eOsMemoryRead;
    }

    if (bool(flags & PageFlags::eWrite)) {
        access |= eOsMemoryWrite;
    }

    if (bool(flags & PageFlags::eExecute)) {
        access |= eOsMemoryExecute;
    }

    return access;
}

static void AddVmemSystemCalls() {
    AddSystemCall(eOsCallAddressSpaceCreate, [](uint64_t userCreateInfo, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        OsAddressSpaceCreateInfo createInfo{};
        if (OsStatus status = km::CopyUserMemory(userCreateInfo, sizeof(createInfo), &createInfo)) {
            return CallError(status);
        }

        PageFlags flags = PageFlags::eUser;
        if (OsStatus status = GetMemoryAccess(createInfo.Access, &flags)) {
            return CallError(status);
        }

        Process * process;
        if (createInfo.Process != OS_HANDLE_INVALID) {
            process = gSystemObjects->getProcess(ProcessId(createInfo.Process & 0x00FF'FFFF'FFFF'FFFF));
        } else {
            process = GetCurrentProcess();
        }

        if (process == nullptr) {
            return CallError(OsStatusNotFound);
        }

        stdx::String name;
        if (OsStatus status = km::CopyUserRange(createInfo.NameFront, createInfo.NameBack, &name, kMaxPathSize)) {
            return CallError(status);
        }

        AddressMapping mapping = gMemory->userAllocate(createInfo.Size, flags, MemoryType::eWriteBack);

        if (mapping.isEmpty()) {
            return CallError(OsStatusOutOfMemory);
        }

        AddressSpace * addressSpace = gSystemObjects->createAddressSpace(std::move(name), mapping, flags, MemoryType::eWriteBack, process);
        return CallOk(addressSpace->id());
    });

    AddSystemCall(eOsCallAddressSpaceStat, [](uint64_t id, uint64_t userStat, uint64_t, uint64_t) -> OsCallResult {
        AddressSpace * space = gSystemObjects->getAddressSpace(AddressSpaceId(id & 0x00FF'FFFF'FFFF'FFFF));
        km::AddressMapping mapping = space->mapping;

        OsAddressSpaceInfo stat {
            .Base = (OsAnyPointer)mapping.vaddr,
            .Size = mapping.size,

            .Access = MakeMemoryAccess(space->flags),
        };

        if (OsStatus status = km::WriteUserMemory((void*)userStat, &stat, sizeof(stat))) {
            return CallError(status);
        }

        return CallOk(0zu);
    });
}

static void StartupSmp(const acpi::AcpiTables& rsdt) {
    //
    // Create the scheduler and system objects before we startup SMP so that
    // the AP cores have a scheduler to attach to.
    //
    gScheduler = new Scheduler();
    gSystemObjects = new SystemObjects();

    std::atomic_flag launchScheduler = ATOMIC_FLAG_INIT;

    if constexpr (kEnableSmp) {
        //
        // We provide an atomic flag to the AP cores that we use to signal when the
        // scheduler is ready to be used. The scheduler requires the system to switch
        // to using cpu local isr tables, which must happen after smp startup.
        //
        InitSmp(*GetSystemMemory(), GetCpuLocalApic(), rsdt, [&launchScheduler](LocalIsrTable *ist, IApic *apic) {
            while (!launchScheduler.test()) {
                _mm_pause();
            }

            KmHalt();

            km::ScheduleWork(ist, apic);
        });
    }

    SetDebugLogLock(DebugLogLockType::eRecursiveSpinLock);

    //
    // Setup gdt that contains a TSS for this core and configure cpu state
    // required to enter usermode on this thread.
    //
    SetupApGdt();
    km::SetupUserMode(GetSystemMemory());

    if constexpr (kEnableSmp) {
        //
        // Signal that the scheduler is now ready to accept work items.
        //
        launchScheduler.test_and_set();
    }
}

static constexpr size_t kKernelStackSize = 0x4000;

static OsStatus LaunchThread(OsStatus(*entry)(void*), void *arg, stdx::String name) {
    stdx::String stackName = name + " STACK";
    Process * process = GetCurrentProcess();
    Thread * thread = gSystemObjects->createThread(std::move(name), process);

    PageFlags flags = PageFlags::eData;
    MemoryType type = MemoryType::eWriteBack;
    km::AddressMapping mapping = gMemory->kernelAllocate(kKernelStackSize, flags, type);
    AddressSpace * stackSpace = gSystemObjects->createAddressSpace(std::move(stackName), mapping, flags, type, process);

    thread->state = km::IsrContext {
        .rdi = (uintptr_t)arg,
        .rbp = (uintptr_t)mapping.vaddr + kKernelStackSize,
        .rip = (uintptr_t)entry,
        .cs = SystemGdt::eLongModeCode * 0x8,
        .rflags = 0x202,
        .rsp = (uintptr_t)mapping.vaddr + kKernelStackSize,
        .ss = SystemGdt::eLongModeData * 0x8,
    };

    gScheduler->addWorkItem(thread);

    return OsStatusSuccess;
}

static OsStatus NotificationWork(void *arg) {
    NotificationStream *stream = (NotificationStream*)arg;
    while (true) {
        stream->processAll();
    }

    return OsStatusSuccess;
}

static OsStatus LaunchInitProcess(ProcessLaunch *launch) {
    std::unique_ptr<vfs2::IVfsNodeHandle> init = nullptr;
    if (OsStatus status = gVfsRoot->open(vfs2::BuildPath("Init", "init.elf"), std::out_ptr(init))) {
        KmDebugMessage("[VFS] Failed to find '/Init/init.elf' ", status, "\n");
        KM_PANIC("Failed to open init process.");
    }

    return LoadElf(std::move(init), *gMemory, *gSystemObjects, launch);
}

static OsStatus KernelMasterTask() {
    KmDebugMessage("[INIT] Kernel master task.\n");

    LaunchThread(&NotificationWork, gNotificationStream, "NOTIFY");

    ProcessLaunch init{};
    if (OsStatus status = LaunchInitProcess(&init)) {
        KmDebugMessage("[INIT] Failed to launch init process: ", status, "\n");
        KM_PANIC("Failed to launch init process.");
    }

    gScheduler->addWorkItem(init.main);

    KmDebugMessage("[INIT] Dispatch NOTIFY thread.\n");

    for (int i = 0; i < 100; i++) {
        LaunchThread(&NotificationWork, gNotificationStream, "STRESS");
    }

    KmDebugMessage("[INIT] Dispatch STRESS threads.\n");

    while (true) {
        //
        // Spin forever for now, in the future this task will handle
        // top level kernel events.
        //
    }

    return OsStatusSuccess;
}

static void ConfigurePs2Controller(const acpi::AcpiTables& rsdt, IoApicSet& ioApicSet, const IApic *apic, LocalIsrTable *ist) {
    static hid::Ps2Controller ps2Controller;

    bool has8042 = rsdt.has8042Controller();

    if (has8042) {
        hid::Ps2ControllerResult result = hid::EnablePs2Controller();
        KmDebugMessage("[INIT] PS/2 controller: ", result.status, "\n");
        if (result.status == hid::Ps2ControllerStatus::eOk) {
            ps2Controller = result.controller;

            KmDebugMessage("[INIT] PS/2 keyboard: ", present(ps2Controller.hasKeyboard()), "\n");
            KmDebugMessage("[INIT] PS/2 mouse: ", present(ps2Controller.hasMouse()), "\n");
        } else {
            return;
        }
    } else {
        KmDebugMessage("[INIT] No PS/2 controller found.\n");
        return;
    }

    if (ps2Controller.hasKeyboard()) {
        static hid::Ps2Device keyboard = ps2Controller.keyboard();
        static hid::Ps2Device mouse = ps2Controller.mouse();

        ps2Controller.setMouseSampleRate(10);
        ps2Controller.setMouseResolution(1);

        hid::InitPs2HidStream(gNotificationStream);

        hid::InstallPs2KeyboardIsr(ioApicSet, ps2Controller, apic, ist);
        hid::InstallPs2MouseIsr(ioApicSet, ps2Controller, apic, ist);

        mouse.disable();
        keyboard.enable();
        ps2Controller.enableIrqs(true, true);
    }

    vfs2::VfsPath hidPs2DevicePath{OS_DEVICE_PS2_KEYBOARD};

    {
        vfs2::IVfsNode *node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(hidPs2DevicePath.parent(), &node)) {
            KmDebugMessage("[VFS] Failed to create ", hidPs2DevicePath.parent(), " folder: ", status, "\n");
            KM_PANIC("Failed to create keyboar device folder.");
        }
    }

    {
        dev::HidKeyboardDevice *device = new dev::HidKeyboardDevice();
        if (OsStatus status = gVfsRoot->mkdevice(hidPs2DevicePath, device)) {
            KmDebugMessage("[VFS] Failed to create ", hidPs2DevicePath, " device: ", status, "\n");
            KM_PANIC("Failed to create keyboard device.");
        }

        gNotificationStream->subscribe(hid::GetHidPs2Topic(), device);
    }
}

static void CreateDisplayDevice() {
    vfs2::VfsPath ddiPath{OS_DEVICE_DDI_RAMFB};

    {
        vfs2::IVfsNode *node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(ddiPath.parent(), &node)) {
            KmDebugMessage("[VFS] Failed to create ", ddiPath.parent(), " folder: ", status, "\n");
            KM_PANIC("Failed to create display device folder.");
        }
    }

    {
        dev::DisplayDevice *device = new dev::DisplayDevice(gDirectTerminalLog.get().display());
        if (OsStatus status = gVfsRoot->mkdevice(ddiPath, device)) {
            KmDebugMessage("[VFS] Failed to create ", ddiPath, " device: ", status, "\n");
            KM_PANIC("Failed to create display device.");
        }
    }
}

[[noreturn]]
static void LaunchKernelProcess(LocalIsrTable *table, IApic *apic) {
    Process * process = gSystemObjects->createProcess("SYSTEM", x64::Privilege::eSupervisor);
    Thread * thread = gSystemObjects->createThread("MASTER", process);

    PageFlags flags = PageFlags::eData;
    MemoryType type = MemoryType::eWriteBack;
    km::AddressMapping mapping = gMemory->kernelAllocate(kKernelStackSize, flags, type);
    AddressSpace * stackSpace = gSystemObjects->createAddressSpace("MASTER STACK", mapping, flags, type, process);

    thread->stack = stackSpace;
    thread->state = km::IsrContext {
        .rbp = (uintptr_t)(thread->stack.get() + kKernelStackSize),
        .rip = (uintptr_t)&KernelMasterTask,
        .cs = SystemGdt::eLongModeCode * 0x8,
        .rflags = 0x202,
        .rsp = (uintptr_t)(thread->stack.get() + kKernelStackSize),
        .ss = SystemGdt::eLongModeData * 0x8,
    };

    gScheduler->addWorkItem(thread);
    ScheduleWork(table, apic);

    KM_PANIC("Failed to schedule work.");
}

void LaunchKernel(boot::LaunchInfo launch) {
    NormalizeProcessorState();
    SetDebugLogLock(DebugLogLockType::eNone);
    InitBootTerminal(launch.framebuffers);

    ProcessorInfo processor = GetProcessorInfo();
    InitPortDelay();

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
    };

    InitSerialPort(com1Info);

    gBootGdt = GetBootGdt();
    SetupInitialGdt();

    //
    // Once we have the initial gdt setup we create the global allocator.
    // The IDT depends on the allocator to create its global ISR table.
    //
    Stage1MemoryInfo stage1 = InitStage1Memory(launch, processor);
    Stage2MemoryInfo *stage2 = InitStage2Memory(launch, processor, stage1);
    gMemory = stage2->memory;

    static constexpr size_t kSchedulerMemorySize = sm::megabytes(1).bytes();
    InitSchedulerMemory(aligned_alloc(x64::kPageSize, kSchedulerMemorySize), kSchedulerMemorySize);

    //
    // I need to disable the PIC before enabling interrupts otherwise I
    // get flooded by timer interrupts.
    //
    Disable8259Pic();

    InitStage1Idt(SystemGdt::eLongModeCode);
    EnableInterrupts();

    bool useX2Apic = kUseX2Apic && processor.has2xApic;

    auto [lapic, spuriousInt] = EnableBootApic(*stage2->memory, useX2Apic);

    acpi::AcpiTables rsdt = acpi::InitAcpi(launch.rsdpAddress, *stage2->memory);
    uint32_t ioApicCount = rsdt.ioApicCount();
    KM_CHECK(ioApicCount > 0, "No IOAPICs found.");
    IoApicSet ioApicSet{ rsdt.madt(), *stage2->memory };

    StartupSmp(rsdt);

    km::LocalIsrTable *ist = GetLocalIsrTable();

    MountRootVfs();
    MountVolatileFolder();
    MountInitArchive(launch.initrd, *stage2->memory);

    AddDebugSystemCalls();
    AddVfsSystemCalls();
    AddDeviceSystemCalls();
    AddThreadSystemCalls();
    AddVmemSystemCalls();

    CreateNotificationQueue();

    ConfigurePs2Controller(rsdt, ioApicSet, lapic.pointer(), ist);
    CreateDisplayDevice();

    LaunchKernelProcess(ist, lapic.pointer());

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
