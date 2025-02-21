// SPDX-License-Identifier: GPL-3.0-only

#include <numeric>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "apic.hpp"

#include "arch/abi.hpp"
#include "arch/cr0.hpp"
#include "arch/cr4.hpp"

#include "acpi/acpi.hpp"

#include <bezos/subsystem/hid.h>
#include <bezos/facility/device.h>
#include <bezos/subsystem/ddi.h>
#include <bezos/facility/vmem.h>

#include "cmos.hpp"
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
#include "pat.hpp"
#include "pit.hpp"
#include "processor.hpp"
#include "process/schedule.hpp"
#include "smp.hpp"
#include "std/spinlock.hpp"
#include "std/static_vector.hpp"
#include "syscall.hpp"
#include "thread.hpp"
#include "uart.hpp"
#include "smbios.hpp"
#include "pci/pci.hpp"
#include "user/user.hpp"

#include "memory/layout.hpp"
#include "memory/allocator.hpp"

#include "allocator/tlsf.hpp"

#include "arch/intrin.hpp"
#include "can.hpp"

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
static constexpr bool kEmitAddrToLine = true;

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

class DebugPortLog final : public IOutStream {
    void write(stdx::StringView message) override {
        for (char c : message) {
            __outbyte(0xE9, c);
        }
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
constinit static DebugPortLog gDebugPortLog;

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

// qemu e9 port check - i think bochs does something else
static bool KmTestDebugPort(void) {
    return __inbyte(0xE9) == 0xE9;
}

constinit km::CpuLocal<SystemGdt> km::tlsSystemGdt;
constinit km::CpuLocal<x64::TaskStateSegment> km::tlsTaskState;

static constinit x64::TaskStateSegment gBootTss{};
static constinit SystemGdt gBootGdt{};

static constinit SerialPort gCanBusPort;
static constinit CanBus gCanBus{nullptr};

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

static PageMemoryTypeLayout GetDefaultPatLayout(void) {
    enum {
        kEntryWriteBack,
        kEntryWriteThrough,
        kEntryUncachedOverridable,
        kEntryUncached,
        kEntryWriteCombined,
        kEntryWriteProtect,
    };

    return PageMemoryTypeLayout {
        .deferred = kEntryUncachedOverridable,
        .uncached = kEntryUncached,
        .writeCombined = kEntryWriteCombined,
        .writeThrough = kEntryWriteThrough,
        .writeProtect = kEntryWriteProtect,
        .writeBack = kEntryWriteBack,
    };
}

static PageMemoryTypeLayout SetupPat(void) {
    if (!x64::HasPatSupport()) {
        return PageMemoryTypeLayout { };
    }

    x64::PageAttributeTable pat = x64::PageAttributeTable::get();

    for (uint8_t i = 0; i < pat.count(); i++) {
        km::MemoryType type = pat.getEntry(i);
        KmDebugMessage("[INIT] PAT[", i, "]: ", type, "\n");
    }

    auto layout = GetDefaultPatLayout();

    pat.setEntry(layout.uncached, MemoryType::eUncached);
    pat.setEntry(layout.writeCombined, MemoryType::eWriteCombine);
    pat.setEntry(layout.writeProtect, MemoryType::eWriteThrough);
    pat.setEntry(layout.writeBack, MemoryType::eWriteBack);
    pat.setEntry(layout.writeProtect, MemoryType::eWriteProtect);
    pat.setEntry(layout.deferred, MemoryType::eUncachedOverridable);

    // PAT[6] and PAT[7] are unused for now, so just set them to UC-
    pat.setEntry(6, MemoryType::eUncachedOverridable);
    pat.setEntry(7, MemoryType::eUncachedOverridable);

    return layout;
}

[[maybe_unused]]
static void SetupMtrrs(x64::MemoryTypeRanges& mtrrs, const km::PageBuilder& pm) {
    mtrrs.setDefaultType(km::MemoryType::eWriteBack);

    // Mark all fixed MTRRs as write-back
    if (mtrrs.fixedMtrrSupported()) {
        for (uint8_t i = 0; i < mtrrs.fixedMtrrCount(); i++) {
            mtrrs.setFixedMtrr(i, km::MemoryType::eWriteBack);
        }
    }

    // Disable all variable mtrrs
    for (uint8_t i = 0; i < mtrrs.variableMtrrCount(); i++) {
        mtrrs.setVariableMtrr(i, pm, km::MemoryType::eUncached, nullptr, 0, false);
    }
}

static void WriteMtrrs(const km::PageBuilder& pm) {
    if (!x64::HasMtrrSupport()) {
        return;
    }

    x64::MemoryTypeRanges mtrrs = x64::MemoryTypeRanges::get();

    KmDebugMessage("[INIT] MTRR fixed support: ", present(mtrrs.fixedMtrrSupported()), "\n");
    KmDebugMessage("[INIT] MTRR fixed enabled: ", enabled(mtrrs.fixedMtrrEnabled()), "\n");
    KmDebugMessage("[INIT] MTRR fixed count: ", mtrrs.fixedMtrrCount(), "\n");
    KmDebugMessage("[INIT] Default MTRR type: ", mtrrs.defaultType(), "\n");

    KmDebugMessage("[INIT] MTRR variable supported: ", enabled(HasVariableMtrrSupport(mtrrs)), "\n");
    KmDebugMessage("[INIT] MTRR variable count: ", mtrrs.variableMtrrCount(), "\n");
    KmDebugMessage("[INIT] MTRR write combining: ", enabled(mtrrs.hasWriteCombining()), "\n");
    KmDebugMessage("[INIT] MTRRs enabled: ", enabled(mtrrs.enabled()), "\n");

    if (mtrrs.fixedMtrrSupported()) {
        for (uint8_t i = 0; i < 11; i++) {
            KmDebugMessage("[INIT] Fixed MTRR[", rpad(2) + i, "]: ");
            for (uint8_t j = 0; j < 8; j++) {
                if (j != 0) {
                    KmDebugMessage("| ");
                }

                KmDebugMessage(mtrrs.fixedMtrr(i), " ");
            }
            KmDebugMessage("\n");
        }
    }

    if (HasVariableMtrrSupport(mtrrs)) {
        for (uint8_t i = 0; i < mtrrs.variableMtrrCount(); i++) {
            x64::VariableMtrr mtrr = mtrrs.variableMtrr(i);
            KmDebugMessage("[INIT] Variable MTRR[", i, "]: ", mtrr.type(), ", address: ", mtrr.baseAddress(pm), ", mask: ", Hex(mtrr.addressMask(pm)).pad(16, '0'), "\n");
        }
    }

    // SetupMtrrs(mtrrs, pm);
}

static void WriteMemoryMap(const boot::MemoryMap& memmap) {
    KmDebugMessage("[INIT] ", memmap.regions.size(), " memory map entries.\n");

    KmDebugMessage("| Entry | Address            | Size               | Type\n");
    KmDebugMessage("|-------+--------------------+--------------------+-----------------------\n");

    for (size_t i = 0; i < memmap.regions.size(); i++) {
        boot::MemoryRegion entry = memmap.regions[i];
        MemoryRange range = entry.range;

        KmDebugMessage("| ", Int(i).pad(4, '0'), "  | ", Hex(range.front.address).pad(16, '0'), " | ", rpad(18) + sm::bytes(range.size()), " | ", entry.type, "\n");
    }

    KmDebugMessage("[INIT] Usable memory: ", memmap.usableMemory(), ", Reclaimable memory: ", memmap.reclaimableMemory(), "\n");
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
        WriteMemoryMap(boot::MemoryMap{memory.memmap});
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

static size_t GetEarlyMemorySize(const boot::MemoryMap& memmap) {
    size_t size = 0;
    for (const boot::MemoryRegion& entry : memmap.regions) {
        if (!entry.isUsable() && !entry.isReclaimable())
            continue;

        if (IsLowMemory(entry.range))
            continue;

        size += entry.size();
    }

    // should only need around 1% of total memory for the early allocator
    return sm::roundup(size / 100, x64::kPageSize);
}

static boot::MemoryRegion FindEarlyAllocatorRegion(const boot::MemoryMap& memmap, size_t size) {
    for (const boot::MemoryRegion& entry : memmap.regions) {
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

static std::tuple<MemoryMap*, km::AddressMapping> CreateEarlyAllocator(const boot::MemoryMap& memmap, uintptr_t hhdmOffset) {
    size_t earlyAllocSize = std::max(GetEarlyMemorySize(memmap), kStage1AllocMinSize);

    boot::MemoryRegion region = FindEarlyAllocatorRegion(memmap, earlyAllocSize);
    km::MemoryRange range = { region.range.front, region.range.front + earlyAllocSize };

    void *vaddr = std::bit_cast<void*>(region.range.front + hhdmOffset);
    MemoryMap *memory = AllocateEarlyMemory(vaddr, earlyAllocSize);
    std::copy(memmap.regions.begin(), memmap.regions.end(), std::back_inserter(memory->memmap));

    memory->insert({ boot::MemoryRegion::eKernelRuntimeData, range });

    return std::make_tuple(memory, km::AddressMapping { vaddr, range.front, earlyAllocSize });
}

static void MapDisplayRegions(PageTableManager& vmm, std::span<const boot::FrameBuffer> framebuffers, VirtualRange range) {
    uintptr_t framebufferBase = (uintptr_t)range.front;
    for (const boot::FrameBuffer& framebuffer : framebuffers) {
        // remap the framebuffer into its final location
        km::AddressMapping fb = { (void*)framebufferBase, framebuffer.paddr, framebuffer.size() };
        vmm.map(fb, PageFlags::eData, MemoryType::eWriteCombine);

        framebufferBase += framebuffer.size();
    }
}

static void MapKernelRegions(PageTableManager &vmm, const KernelLayout& layout) {
    auto [kernelVirtualBase, kernelPhysicalBase, _] = layout.kernel;
    KmMapKernel(vmm, kernelPhysicalBase, kernelVirtualBase);
}

static void MapDataRegion(PageTableManager &vmm, km::AddressMapping mapping) {
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

    WriteMtrrs(pm);

    auto [earlyMemory, earlyRegion] = CreateEarlyAllocator(launch.memmap, launch.hhdmOffset);

    KernelLayout layout = BuildKernelLayout(
        processor.maxvaddr,
        launch.kernelPhysicalBase,
        launch.kernelVirtualBase,
        *earlyMemory,
        launch.framebuffers
    );

    WriteMemoryMap(boot::MemoryMap{earlyMemory->memmap});

    mem::IAllocator *alloc = &earlyMemory->allocator;

    boot::FrameBuffer *framebuffers = CloneFrameBuffers(alloc, launch.framebuffers);

    PageTableManager vmm(&pm, &earlyMemory->allocator);

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
    mem::TlsfAllocator allocator;
    KernelLayout layout;
};

static constinit mem::IAllocator *gAllocator = nullptr;
static constinit stdx::SpinLock gAllocatorLock;

extern "C" void *malloc(size_t size) {
    stdx::LockGuard _(gAllocatorLock);
    return gAllocator->allocate(size);
}

extern "C" void *realloc(void *old, size_t size) {
    stdx::LockGuard _(gAllocatorLock);
    return gAllocator->reallocate(old, 0, size);
}

extern "C" void free(void *ptr) {
    stdx::LockGuard _(gAllocatorLock);
    gAllocator->deallocate(ptr, 0);
}

extern "C" void *aligned_alloc(size_t alignment, size_t size) {
    stdx::LockGuard _(gAllocatorLock);
    return gAllocator->allocateAligned(size, alignment);
}

static void InitGlobalAllocator(mem::IAllocator *allocator) {
    stdx::LockGuard _(gAllocatorLock);
    gAllocator = allocator;
}

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

    // Create the global memory allocator
    mem::TlsfAllocator alloc{(void*)layout.committed.vaddr, layout.committed.size};

    Stage2MemoryInfo *stage2 = alloc.construct<Stage2MemoryInfo>();
    stage2->allocator = std::move(alloc);

    InitGlobalAllocator(&stage2->allocator);

    SystemMemory *memory = stage2->allocator.construct<SystemMemory>(boot::MemoryMap{earlyMemory->memmap}, stage1.layout.system, pm, &stage2->allocator);
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

static void DumpIsrState(const km::IsrContext *context) {
    KmDebugMessageUnlocked("| Register | Value\n");
    KmDebugMessageUnlocked("|----------+------\n");
    KmDebugMessageUnlocked("| %RAX     | ", Hex(context->rax).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RBX     | ", Hex(context->rbx).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RCX     | ", Hex(context->rcx).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RDX     | ", Hex(context->rdx).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RDI     | ", Hex(context->rdi).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RSI     | ", Hex(context->rsi).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R8      | ", Hex(context->r8).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R9      | ", Hex(context->r9).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R10     | ", Hex(context->r10).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R11     | ", Hex(context->r11).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R12     | ", Hex(context->r12).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R13     | ", Hex(context->r13).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R14     | ", Hex(context->r14).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R15     | ", Hex(context->r15).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RBP     | ", Hex(context->rbp).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RIP     | ", Hex(context->rip).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %CS      | ", Hex(context->cs).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RFLAGS  | ", Hex(context->rflags).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RSP     | ", Hex(context->rsp).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %SS      | ", Hex(context->ss).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| Vector   | ", Hex(context->vector).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| Error    | ", Hex(context->error).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("\n");

    KmDebugMessageUnlocked("| MSR                 | Value\n");
    KmDebugMessageUnlocked("|---------------------+------\n");
    KmDebugMessageUnlocked("| IA32_GS_BASE        | ", Hex(kGsBase.load()).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| IA32_FS_BASE        | ", Hex(kFsBase.load()).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| IA32_KERNEL_GS_BASE | ", Hex(kKernelGsBase.load()).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("\n");
}

static void DumpStackTrace(const km::IsrContext *context) {
    KmDebugMessageUnlocked("|----Stack trace-----+----------------\n");
    KmDebugMessageUnlocked("| Frame              | Program Counter\n");
    KmDebugMessageUnlocked("|--------------------+----------------\n");
    x64::WalkStackFrames((void**)context->rbp, [](void **frame, void *pc) {
        KmDebugMessageUnlocked("| ", (void*)frame, " | ", pc, "\n");
    });

    if (kEmitAddrToLine) {
        KmDebugMessageUnlocked("llvm-addr2line -e ./build/kernel/bezos-limine");

        x64::WalkStackFrames((void**)context->rbp, [](void **, void *pc) {
            KmDebugMessageUnlocked(" ", pc);
        });

        KmDebugMessageUnlocked("\n");
    }
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

    km::InitCpuLocalRegion(memory);
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

static void UpdateSerialPort(ComPortInfo info) {
    info.skipLoopbackTest = true;
    if (OpenSerialResult com = OpenSerial(info); com.status == SerialPortStatus::eOk) {
        gSerialLog = SerialLog(com.port);
        gLogTargets.add(&gSerialLog);
    }
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

static SerialPortStatus InitCanSerialPort(ComPortInfo info) {
    if (OpenSerialResult com1 = OpenSerial(info)) {
        return com1.status;
    } else {
        gCanBusPort = com1.port;
        gCanBus = CanBus(&gCanBusPort);
        gCanBus.sendDataFrame(0x123, 0x9988776655443322);
        return SerialPortStatus::eOk;
    }
}

static void UpdateCanSerialPort(ComPortInfo info) {
    info.skipLoopbackTest = true;
    if (OpenSerialResult com1 = OpenSerial(info); com1.status == SerialPortStatus::eOk) {
        gCanBusPort = com1.port;
        gCanBus = CanBus(&gCanBusPort);
    }
}

static void DumpIsrContext(const km::IsrContext *context, stdx::StringView message) {
    if (km::IsCpuStorageSetup()) {
        KmDebugMessageUnlocked("\n[BUG] ", message, " - On ", km::GetCurrentCoreId(), "\n");
    } else {
        KmDebugMessageUnlocked("\n[BUG] ", message, "\n");
    }

    DumpIsrState(context);
}

static void InstallExceptionHandlers(LocalIsrTable *ist) {
    ist->install(isr::DE, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Divide by zero (#DE)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::NMI, [](km::IsrContext *context) -> km::IsrContext {
        KmDebugMessageUnlocked("[INT] Non-maskable interrupt (#NM)\n");
        DumpIsrState(context);
        return *context;
    });

    ist->install(isr::UD, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Invalid opcode (#UD)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::DF, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Double fault (#DF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::GP, [](km::IsrContext *context) -> km::IsrContext {
        DisableInterrupts();

        DumpIsrContext(context, "General protection fault (#GP)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::PF, [](km::IsrContext *context) -> km::IsrContext {
        KmDebugMessageUnlocked("[BUG] CR2: ", Hex(__get_cr2()).pad(16, '0'), "\n");
        DumpIsrContext(context, "Page fault (#PF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });
}

static void InitPortDelay(const std::optional<HypervisorInfo>& hvInfo) {
    bool isKvm = hvInfo.transform([](const HypervisorInfo& hv) { return hv.isKvm(); }).value_or(false);
    x64::PortDelay delay = isKvm ? x64::PortDelay::eNone : x64::PortDelay::ePostCode;

    KmSetPortDelayMethod(delay);
}

static void LogSystemInfo(
    const boot::LaunchInfo& launch,
    std::optional<HypervisorInfo> hvInfo,
    ProcessorInfo processor,
    bool hasDebugPort,
    SerialPortStatus com1Status,
    const ComPortInfo& com1Info,
    SerialPortStatus com2Status,
    const ComPortInfo& com2Info) {
    KmDebugMessage("[INIT] CR0: ", x64::Cr0::load(), "\n");
    KmDebugMessage("[INIT] CR4: ", x64::Cr4::load(), "\n");

    KmDebugMessage("[INIT] System report.\n");
    KmDebugMessage("| Component     | Property             | Status\n");
    KmDebugMessage("|---------------+----------------------+-------\n");

    if (hvInfo.has_value()) {
        KmDebugMessage("| /SYS/HV       | Hypervisor           | ", hvInfo->name, "\n");
        KmDebugMessage("| /SYS/HV       | Max HvCPUID leaf     | ", Hex(hvInfo->maxleaf).pad(8, '0'), "\n");
        KmDebugMessage("| /SYS/HV       | e9 debug port        | ", enabled(hasDebugPort), "\n");
    } else {
        KmDebugMessage("| /SYS/HV       | Hypervisor           | Not present\n");
        KmDebugMessage("| /SYS/HV       | Max HvCPUID leaf     | Not applicable\n");
        KmDebugMessage("| /SYS/HV       | e9 debug port        | Not applicable\n");
    }

    KmDebugMessage("| /SYS/MB/CPU0  | Vendor               | ", processor.vendor, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Model name           | ", processor.brand, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Max CPUID leaf       | ", Hex(processor.maxleaf).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Max physical address | ", processor.maxpaddr, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Max virtual address  | ", processor.maxvaddr, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | TSC ratio            | ", processor.coreClock.tsc, "/", processor.coreClock.core, "\n");

    if (processor.hasNominalFrequency()) {
        KmDebugMessage("| /SYS/MB/CPU0  | Bus clock            | ", processor.busClock, "hz\n");
    } else {
        KmDebugMessage("| /SYS/MB/CPU0  | Bus clock            | Not available\n");
    }

    KmDebugMessage("| /SYS/MB/CPU0  | Base frequency       | ", processor.baseFrequency, "mhz\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Max frequency        | ", processor.maxFrequency, "mhz\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Bus frequency        | ", processor.busFrequency, "mhz\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Local APIC           | ", present(processor.hasLocalApic), "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | 2x APIC              | ", present(processor.has2xApic), "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | SMBIOS 32 address    | ", launch.smbios32Address, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | SMBIOS 64 address    | ", launch.smbios64Address, "\n");

    for (size_t i = 0; i < launch.framebuffers.size(); i++) {
        const boot::FrameBuffer& display = launch.framebuffers[i];
        KmDebugMessage("| /SYS/VIDEO", i, "   | Display resolution   | ", display.width, "x", display.height, "x", display.bpp, "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Framebuffer size     | ", sm::bytes(display.size()), "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Framebuffer address  | ", display.vaddr, "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Display pitch        | ", display.pitch, "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | EDID                 | ", display.edid, "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Red channel          | (mask=", display.redMaskSize, ",shift=", display.redMaskShift, ")\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Green channel        | (mask=", display.greenMaskSize, ",shift=", display.greenMaskShift, ")\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Blue channel         | (mask=", display.blueMaskSize, ",shift=", display.blueMaskShift, ")\n");
    }

    KmDebugMessage("| /SYS/MB/COM1  | Status               | ", com1Status, "\n");
    KmDebugMessage("| /SYS/MB/COM1  | Port                 | ", Hex(com1Info.port), "\n");
    KmDebugMessage("| /SYS/MB/COM1  | Baud rate            | ", km::com::kBaudRate / com1Info.divisor, "\n");
    KmDebugMessage("| /SYS/MB/COM2  | Status               | ", com2Status, "\n");
    KmDebugMessage("| /SYS/MB/COM2  | Port                 | ", Hex(com2Info.port), "\n");
    KmDebugMessage("| /SYS/MB/COM2  | Baud rate            | ", km::com::kBaudRate / com2Info.divisor, "\n");

    KmDebugMessage("| /BOOT         | Stack                | ", launch.stack, "\n");
    KmDebugMessage("| /BOOT         | Kernel virtual       | ", launch.kernelVirtualBase, "\n");
    KmDebugMessage("| /BOOT         | Kernel physical      | ", launch.kernelPhysicalBase, "\n");
    KmDebugMessage("| /BOOT         | INITRD               | ", launch.initrd, "\n");
    KmDebugMessage("| /BOOT         | HHDM offset          | ", Hex(launch.hhdmOffset).pad(16, '0'), "\n");
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
    InstallExceptionHandlers(GetLocalIsrTable());
    SetupInterruptStacks(cs);

    if (kSelfTestIdt) {
        km::LocalIsrTable *ist = GetLocalIsrTable();
        IsrCallback old = ist->install(64, [](km::IsrContext *context) -> km::IsrContext {
            KmDebugMessage("[SELFTEST] Handled isr: ", context->vector, "\n");
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
    gNotificationStream = new NotificationStream();
}

static const void *TranslateUserPointer(const void *userAddress) {
    PageTableManager& pt = gMemory->pt;
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

static OsStatus LaunchInitProcess(ProcessLaunch *launch) {
    std::unique_ptr<vfs2::IVfsNodeHandle> init = nullptr;
    if (OsStatus status = gVfsRoot->open(vfs2::BuildPath("Init", "init.elf"), std::out_ptr(init))) {
        KmDebugMessage("[VFS] Failed to find '/Init/init.elf' ", status, "\n");
        KM_PANIC("Failed to open init process.");
    }

    return LoadElf(std::move(init), *gMemory, *gSystemObjects, launch);
}

static std::tuple<std::optional<HypervisorInfo>, bool> QueryHostHypervisor() {
    std::optional<HypervisorInfo> hvInfo = KmGetHypervisorInfo();
    bool hasDebugPort = false;

    if (hvInfo.transform([](const HypervisorInfo& info) { return info.platformHasDebugPort(); }).value_or(false)) {
        hasDebugPort = KmTestDebugPort();
    }

    if (hasDebugPort) {
        gLogTargets.add(&gDebugPortLog);
    }

    return std::make_tuple(hvInfo, hasDebugPort);
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
}

static void AddVmemSystemCalls() {
    AddSystemCall(eOsCallAddressSpaceCreate, [](uint64_t userCreateInfo, uint64_t, uint64_t, uint64_t) -> OsCallResult {
        OsAddressSpaceCreateInfo createInfo{};
        if (OsStatus status = km::CopyUserMemory(userCreateInfo, sizeof(createInfo), &createInfo)) {
            return CallError(status);
        }

        PageFlags flags = PageFlags::eUser;
        if (createInfo.Access & eOsMemoryRead) {
            flags |= PageFlags::eRead;
            createInfo.Access &= ~eOsMemoryRead;
        }

        if (createInfo.Access & eOsMemoryWrite) {
            flags |= PageFlags::eWrite;
            createInfo.Access &= ~eOsMemoryWrite;
        }

        if (createInfo.Access & eOsMemoryExecute) {
            flags |= PageFlags::eExecute;
            createInfo.Access &= ~eOsMemoryExecute;
        }

        if (createInfo.Access != 0) {
            return CallError(OsStatusInvalidInput);
        }

        stdx::String name;
        if (OsStatus status = km::CopyUserRange(createInfo.NameFront, createInfo.NameBack, &name, kMaxPathSize)) {
            return CallError(status);
        }

        AddressMapping mapping = gMemory->userAllocate(createInfo.Size, flags, MemoryType::eWriteBack);

        if (mapping.isEmpty()) {
            return CallError(OsStatusOutOfMemory);
        }

        AddressSpace *addressSpace = gSystemObjects->createAddressSpace(std::move(name), mapping);
        return CallOk(addressSpace);
    });

    AddSystemCall(eOsCallAddressSpaceStat, [](uint64_t userSpace, uint64_t userStat, uint64_t, uint64_t) -> OsCallResult {
        AddressSpace *space = (AddressSpace*)userSpace;
        OsAddressSpaceInfo stat {
            .Base = (OsAnyPointer)space->mapping.vaddr,
            .Size = space->mapping.size,

            // TODO: Report the actual access.
            .Access = eOsMemoryRead | eOsMemoryWrite | eOsMemoryExecute,
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

    //
    // We provide an atomic flag to the AP cores that we use to signal when the
    // scheduler is ready to be used. The scheduler requires the system to switch
    // to using cpu local isr tables, which must happen after smp startup.
    //
    std::atomic_flag launchScheduler = ATOMIC_FLAG_INIT;
    std::atomic<uint32_t> remaining;
    InitSmp(*gMemory, GetCpuLocalApic(), rsdt, &launchScheduler, &remaining);
    SetDebugLogLock(DebugLogLockType::eRecursiveSpinLock);

    //
    // Setup gdt that contains a TSS for this core and configure cpu state
    // required to enter usermode on this thread.
    //
    SetupApGdt();
    km::SetupUserMode();

    //
    // Signal that the scheduler is now ready to accept work items.
    //
    launchScheduler.test_and_set();

    while (remaining > 0) {
        //
        // Spin until all AP cores have started.
        //
        _mm_pause();
    }
}

static constexpr size_t kKernelStackSize = 0x4000;

static OsStatus LaunchThread(OsStatus(*entry)(void*), void *arg, stdx::String name) {
    Thread *thread = gSystemObjects->createThread(std::move(name), GetCurrentProcess());
    thread->stack = std::unique_ptr<std::byte[]>(new std::byte[kKernelStackSize]);
    thread->state = km::IsrContext {
        .rdi = (uintptr_t)arg,
        .rbp = (uintptr_t)(thread->stack.get() + kKernelStackSize),
        .rip = (uintptr_t)entry,
        .cs = SystemGdt::eLongModeCode * 0x8,
        .rflags = 0x202,
        .rsp = (uintptr_t)(thread->stack.get() + kKernelStackSize),
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

static OsStatus KernelMasterTask() {
    KmDebugMessage("[INIT] Kernel master task.\n");

    LaunchThread(&NotificationWork, gNotificationStream, "NOTIFY");

    ProcessLaunch init{};
    if (OsStatus status = LaunchInitProcess(&init)) {
        KmDebugMessage("[INIT] Failed to launch init process: ", status, "\n");
        KM_PANIC("Failed to launch init process.");
    }

    gScheduler->addWorkItem(init.main);

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
        }
    } else {
        KmDebugMessage("[INIT] No PS/2 controller found.\n");
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
    Process *process = gSystemObjects->createProcess("SYSTEM", x64::Privilege::eSupervisor);
    Thread *thread = gSystemObjects->createThread("MASTER", process);
    thread->stack = std::unique_ptr<std::byte[]>(new std::byte[kKernelStackSize]);
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
}

void LaunchKernel(boot::LaunchInfo launch) {
    NormalizeProcessorState();
    SetDebugLogLock(DebugLogLockType::eNone);
    InitBootTerminal(launch.framebuffers);
    auto [hvInfo, hasDebugPort] = QueryHostHypervisor();

    ProcessorInfo processor = GetProcessorInfo();
    InitPortDelay(hvInfo);

    ComPortInfo com2Info = {
        .port = km::com::kComPort2,
        .divisor = km::com::kBaud9600,
    };

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
    };

    SerialPortStatus com1Status = InitSerialPort(com1Info);
    SerialPortStatus com2Status = InitCanSerialPort(com2Info);

    LogSystemInfo(launch, hvInfo, processor, hasDebugPort, com1Status, com1Info, com2Status, com2Info);

    gBootGdt = GetBootGdt();
    SetupInitialGdt();

    //
    // Once we have the initial gdt setup we create the global allocator.
    // The IDT depends on the allocator to create its global ISR table.
    //
    Stage1MemoryInfo stage1 = InitStage1Memory(launch, processor);
    Stage2MemoryInfo *stage2 = InitStage2Memory(launch, processor, stage1);
    gMemory = stage2->memory;

    //
    // I need to disable the PIC before enabling interrupts otherwise I
    // get flooded by timer interrupts.
    //
    Disable8259Pic();

    InitStage1Idt(SystemGdt::eLongModeCode);
    EnableInterrupts();

    PlatformInfo platform = GetPlatformInfo(launch.smbios32Address, launch.smbios64Address, *stage2->memory);

    //
    // On Oracle VirtualBox the COM1 port is functional but fails the loopback test.
    // If we are running on VirtualBox, retry the serial port initialization without the loopback test.
    //
    if (com1Status == SerialPortStatus::eLoopbackTestFailed && platform.isOracleVirtualBox()) {
        UpdateSerialPort(com1Info);
        UpdateCanSerialPort(com2Info);
    }

    bool useX2Apic = kUseX2Apic && processor.has2xApic;

    auto [lapic, spuriousInt] = EnableBootApic(*stage2->memory, useX2Apic);

    acpi::AcpiTables rsdt = acpi::InitAcpi(launch.rsdpAddress, *stage2->memory);
    const acpi::Fadt *fadt = rsdt.fadt();
    InitCmos(fadt->century);

    uint32_t ioApicCount = rsdt.ioApicCount();
    KM_CHECK(ioApicCount > 0, "No IOAPICs found.");
    IoApicSet ioApicSet{ rsdt.madt(), *stage2->memory };

    std::unique_ptr<pci::IConfigSpace> config{pci::InitConfigSpace(rsdt.mcfg(), *stage2->memory)};
    if (!config) {
        KM_PANIC("Failed to initialize PCI config space.");
    }

    pci::ProbeConfigSpace(config.get(), rsdt.mcfg());

    StartupSmp(rsdt);

    km::LocalIsrTable *ist = GetLocalIsrTable();
    const IsrEntry *timerInt = ist->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();
        return *ctx;
    });
    uint8_t timerIdx = ist->index(timerInt);

    km::InitPit(100 * si::hertz, ioApicSet, lapic.pointer(), timerIdx);

    std::optional<km::HighPrecisionTimer> hpet = km::HighPrecisionTimer::find(rsdt, *stage2->memory);
    if (hpet.has_value()) {
        KmDebugMessage("|---- HPET ----------------\n");
        KmDebugMessage("| Property     | Value\n");
        KmDebugMessage("|--------------+-----------\n");
        KmDebugMessage("| Vendor       | ", hpet->vendor(), "\n");
        KmDebugMessage("| Timer count  | ", hpet->timerCount(), "\n");
        KmDebugMessage("| Clock        | ", uint32_t(hpet->refclk() / si::hertz), "hz\n");
        KmDebugMessage("| Counter size | ", hpet->counterSize() == HpetWidth::DWORD ? "32"_sv : "64"_sv, "\n");
        KmDebugMessage("| Revision     | ", hpet->revision(), "\n");
    }

    DateTime time = ReadCmosClock();
    KmDebugMessage("[INIT] Current time: ", time.year, "-", time.month, "-", time.day, "T", time.hour, ":", time.minute, ":", time.second, "Z\n");

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
