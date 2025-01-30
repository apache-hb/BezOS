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

#include "cmos.hpp"
#include "delay.hpp"
#include "display.hpp"
#include "elf.hpp"
#include "gdt.hpp"
#include "hid/ps2.hpp"
#include "hypervisor.hpp"
#include "isr.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "memory/memory.hpp"
#include "memory/virtual_allocator.hpp"
#include "panic.hpp"
#include "pat.hpp"
#include "pit.hpp"
#include "processor.hpp"
#include "schedule.hpp"
#include "smp.hpp"
#include "std/spinlock.hpp"
#include "std/static_vector.hpp"
#include "syscall.hpp"
#include "thread.hpp"
#include "uart.hpp"
#include "smbios.hpp"
#include "pci.hpp"

#include "memory/layout.hpp"
#include "memory/allocator.hpp"

#include "allocator/tlsf.hpp"

#include "arch/intrin.hpp"
#include "can.hpp"

#include "kernel.hpp"

#include "util/memory.hpp"

using namespace km;
using namespace stdx::literals;

static constexpr bool kUseX2Apic = true;
static constexpr bool kSelfTestIdt = true;
static constexpr bool kSelfTestApic = true;
static constexpr bool kDumpAddr2lineCmd = true;

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

static SystemGdt gBootGdt;

static constinit SerialPort gCanBusPort;
static constinit CanBus gCanBus{nullptr};

void km::SetupInitialGdt(void) {
    InitGdt(gBootGdt.entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
}

void km::SetupApGdt(void) {
    // Save the gs/fs/kgsbase values, as setting the GDT will clear them
    CpuLocalRegisters tls = LoadTlsRegisters();

    tlsTaskState = x64::TaskStateSegment {
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
        mOffset -= sm::roundup(memory, x64::kPageSize);

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
    stdx::Vector<boot::MemoryRegion> memmap;

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
    uintptr_t dataBack = -(1ull << (vaddrbits - 1));
    uintptr_t dataFront = -(1ull << (vaddrbits - 2));

    VirtualRange data = { (void*)dataBack, (void*)dataFront };

    AddressSpaceBuilder builder { dataFront };

    km::MemoryRange committedPhysicalMemory = memory.reserve(kCommittedRegionSize);
    if (committedPhysicalMemory.isEmpty()) {
        WriteMemoryMap(boot::MemoryMap{memory.memmap});
        KmDebugMessage("[INIT] Failed to reserve memory for committed region ", sm::bytes(kCommittedRegionSize), ".\n");
        KM_PANIC("Failed to reserve memory for committed region.");
    }

    VirtualRange committedVirtualRange = builder.reserve(kCommittedRegionSize);

    VirtualRange framebuffers = builder.reserve(framebuffersSize);

    km::AddressMapping committed = { committedVirtualRange.front, committedPhysicalMemory.front, kCommittedRegionSize };

    AddressMapping kernel = { kernelVirtualBase, kernelPhysicalBase, GetKernelSize() };

    KmDebugMessage("[INIT] Kernel layout:\n");
    KmDebugMessage("[INIT] Data         : ", data, "\n");
    KmDebugMessage("[INIT] Committed    : ", committed, "\n");
    KmDebugMessage("[INIT] Framebuffers : ", framebuffers, "\n");
    KmDebugMessage("[INIT] Kernel       : ", kernel, "\n");

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
    km::AddressMapping stackMapping = { (void*)((uintptr_t)stack.vaddr - stack.size), stack.paddr - stack.size, stack.size };
    PageBuilder pm = PageBuilder { processor.maxpaddr, launch.hhdmOffset, pat };

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
    MapDataRegion(vmm, stackMapping);

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

static Stage2MemoryInfo *InitStage2Memory(
    const boot::LaunchInfo& launch,
    const km::ProcessorInfo& processor,
    const Stage1MemoryInfo& stage1
) {
    km::AddressMapping stack = launch.stack;
    KernelLayout layout = stage1.layout;
    std::span<boot::FrameBuffer> framebuffers = stage1.framebuffers;
    MemoryMap *earlyMemory = stage1.earlyMemory;

    PageBuilder pm = PageBuilder { processor.maxpaddr, layout.committedSlide(), GetDefaultPatLayout() };
    km::AddressMapping stackMapping = { (void*)((uintptr_t)stack.vaddr - stack.size), stack.paddr - stack.size, stack.size };

    // Create the global memory allocator
    mem::TlsfAllocator alloc{(void*)layout.committed.vaddr, layout.committed.size};

    Stage2MemoryInfo *stage2 = alloc.construct<Stage2MemoryInfo>();
    stage2->allocator = std::move(alloc);

    SystemMemory *memory = stage2->allocator.construct<SystemMemory>(boot::MemoryMap{earlyMemory->memmap}, stage1.layout.system, pm, &stage2->allocator);
    KM_CHECK(memory != nullptr, "Failed to allocate memory for SystemMemory.");

    stage2->memory = memory;
    stage2->layout = layout;

    // carry forward the mappings made in stage 1
    // that are still required for kernel runtime
    memory->pmm.markUsed(stackMapping.physicalRange());
    memory->pmm.markUsed(layout.committed.physicalRange());

    // initialize our own page tables and remap everything into it
    MapKernelRegions(memory->pt, layout);

    // map the stack again
    MapDataRegion(memory->pt, stackMapping);

    // carry forward the non-pageable memory
    MapDataRegion(memory->pt, layout.committed);

    // remap framebuffers
    MapDisplayRegions(memory->pt, framebuffers, layout.framebuffers);

    // once it is safe to remap the boot memory, do so
    KmUpdateRootPageTable(memory->pager, memory->pt);

    return stage2;
}

static void DumpIsrState(const km::IsrContext *context) {
    KmDebugMessage("| Register | Value\n");
    KmDebugMessage("|----------+------\n");
    KmDebugMessage("| %RAX     | ", Hex(context->rax).pad(16, '0'), "\n");
    KmDebugMessage("| %RBX     | ", Hex(context->rbx).pad(16, '0'), "\n");
    KmDebugMessage("| %RCX     | ", Hex(context->rcx).pad(16, '0'), "\n");
    KmDebugMessage("| %RDX     | ", Hex(context->rdx).pad(16, '0'), "\n");
    KmDebugMessage("| %RDI     | ", Hex(context->rdi).pad(16, '0'), "\n");
    KmDebugMessage("| %RSI     | ", Hex(context->rsi).pad(16, '0'), "\n");
    KmDebugMessage("| %R8      | ", Hex(context->r8).pad(16, '0'), "\n");
    KmDebugMessage("| %R9      | ", Hex(context->r9).pad(16, '0'), "\n");
    KmDebugMessage("| %R10     | ", Hex(context->r10).pad(16, '0'), "\n");
    KmDebugMessage("| %R11     | ", Hex(context->r11).pad(16, '0'), "\n");
    KmDebugMessage("| %R12     | ", Hex(context->r12).pad(16, '0'), "\n");
    KmDebugMessage("| %R13     | ", Hex(context->r13).pad(16, '0'), "\n");
    KmDebugMessage("| %R14     | ", Hex(context->r14).pad(16, '0'), "\n");
    KmDebugMessage("| %R15     | ", Hex(context->r15).pad(16, '0'), "\n");
    KmDebugMessage("| %RBP     | ", Hex(context->rbp).pad(16, '0'), "\n");
    KmDebugMessage("| %RIP     | ", Hex(context->rip).pad(16, '0'), "\n");
    KmDebugMessage("| %CS      | ", Hex(context->cs).pad(16, '0'), "\n");
    KmDebugMessage("| %RFLAGS  | ", Hex(context->rflags).pad(16, '0'), "\n");
    KmDebugMessage("| %RSP     | ", Hex(context->rsp).pad(16, '0'), "\n");
    KmDebugMessage("| %SS      | ", Hex(context->ss).pad(16, '0'), "\n");
    KmDebugMessage("| Vector   | ", Hex(context->vector).pad(16, '0'), "\n");
    KmDebugMessage("| Error    | ", Hex(context->error).pad(16, '0'), "\n");
    KmDebugMessage("\n");

    KmDebugMessage("| MSR                 | Value\n");
    KmDebugMessage("|---------------------+------\n");
    KmDebugMessage("| IA32_GS_BASE        | ", Hex(kGsBase.load()).pad(16, '0'), "\n");
    KmDebugMessage("| IA32_FS_BASE        | ", Hex(kFsBase.load()).pad(16, '0'), "\n");
    KmDebugMessage("| IA32_KERNEL_GS_BASE | ", Hex(kKernelGsBase.load()).pad(16, '0'), "\n");
    KmDebugMessage("\n");
}

static void DumpStackTrace(const km::IsrContext *context) {
    KmDebugMessage("|----Stack trace-----+----------------\n");
    KmDebugMessage("| Frame              | Program Counter\n");
    KmDebugMessage("|--------------------+----------------\n");
    x64::WalkStackFrames((void**)context->rbp, [](void **frame, void *pc) {
        KmDebugMessage("| ", (void*)frame, " | ", pc, "\n");
    });

    if (kDumpAddr2lineCmd) {
        KmDebugMessage("llvm-addr2line -e ./build/bezos");
        x64::WalkStackFrames((void**)context->rbp, [](void **, void *pc) {
            KmDebugMessage(" ", pc);
        });
        KmDebugMessage("\n");
    }
}

struct ApicInfo {
    km::Apic apic;
    uint8_t spuriousInt;
};

static ApicInfo EnableBootApic(km::SystemMemory& memory, km::IsrAllocator& isrs, bool useX2Apic) {
    km::Apic pic = InitBspApic(memory, useX2Apic);

    // setup tls now that we have the lapic id

    km::InitCpuLocalRegion(memory);

    SetDebugLogLock(DebugLogLockType::eSpinLock);
    km::InitKernelThread(pic);

    uint8_t spuriousInt = isrs.allocateIsr();
    KmDebugMessage("[INIT] APIC ID: ", pic->id(), ", Version: ", pic->version(), ", Spurious vector: ", spuriousInt, "\n");

    InstallIsrHandler(spuriousInt, [](km::IsrContext *ctx) -> km::IsrContext {
        KmDebugMessage("[ISR] Spurious interrupt: ", ctx->vector, "\n");
        km::IApic *pic = km::GetCpuLocalApic();
        pic->eoi();
        return *ctx;
    });

    pic->setSpuriousVector(spuriousInt);

    pic->enable();

    if (kSelfTestApic) {
        uint8_t isr = isrs.allocateIsr();

        IsrCallback old = InstallIsrHandler(isr, [](km::IsrContext *context) -> km::IsrContext {
            KmDebugMessage("[SELFTEST] Handled isr: ", context->vector, "\n");
            km::IApic *pic = km::GetCpuLocalApic();
            pic->eoi();
            return *context;
        });

        pic->sendIpi(apic::IcrDeliver::eSelf, isr);
        pic->sendIpi(apic::IcrDeliver::eSelf, isr);
        pic->sendIpi(apic::IcrDeliver::eSelf, isr);

        InstallIsrHandler(isr, old);
        isrs.releaseIsr(isr);
    }

    return ApicInfo { pic, spuriousInt };
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
    if (OpenSerialResult com1 = OpenSerial(info); com1.status == SerialPortStatus::eOk) {
        gSerialLog = SerialLog(com1.port);
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
        KmDebugMessage("\n[BUG] ", message, " - On ", km::GetCurrentCoreId(), "\n");
    } else {
        KmDebugMessage("\n[BUG] ", message, "\n");
    }

    DumpIsrState(context);
}

static void InstallExceptionHandlers(void) {
    InstallIsrHandler(0x0, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Divide by zero (#DE)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    InstallIsrHandler(0x2, [](km::IsrContext *context) -> km::IsrContext {
        KmDebugMessage("[INT] Non-maskable interrupt (#NM)\n");
        DumpIsrState(context);
        return *context;
    });

    InstallIsrHandler(0x6, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Invalid opcode (#UD)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    InstallIsrHandler(0x8, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Double fault (#DF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    InstallIsrHandler(0xD, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "General protection fault (#GP)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    InstallIsrHandler(0xE, [](km::IsrContext *context) -> km::IsrContext {
        KmDebugMessage("[BUG] CR2: ", Hex(__get_cr2()).pad(16, '0'), "\n");
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

extern const char _binary_init_elf_start[];
extern const char _binary_init_elf_end[];

static std::span<const uint8_t> GetInitProgram() {
    return { (const uint8_t*)_binary_init_elf_start, (const uint8_t*)_binary_init_elf_end };
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
    KmDebugMessage("[INIT] HHDM: ", Hex(launch.hhdmOffset).pad(16, '0'), "\n");

    KmDebugMessage("[INIT] System report.\n");
    KmDebugMessage("| Component     | Property             | Status\n");
    KmDebugMessage("|---------------+----------------------+-------\n");

    if (hvInfo.has_value()) {
        KmDebugMessage("| /SYS/HV       | Hypervisor           | ", hvInfo->name, "\n");
        KmDebugMessage("| /SYS/HV       | Max CPUID leaf       | ", Hex(hvInfo->maxleaf).pad(8, '0'), "\n");
        KmDebugMessage("| /SYS/HV       | e9 debug port        | ", enabled(hasDebugPort), "\n");
    } else {
        KmDebugMessage("| /SYS/HV       | Hypervisor           | Not present\n");
        KmDebugMessage("| /SYS/HV       | Max CPUID leaf       | 0x00000000\n");
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
}

static km::IsrAllocator InitStage1Idt(uint16_t cs) {
    km::IsrAllocator isrs;
    InitInterrupts(isrs, cs);
    InstallExceptionHandlers();
    EnableInterrupts();

    if (kSelfTestIdt) {
        IsrCallback old = InstallIsrHandler(0x2, [](km::IsrContext *context) -> km::IsrContext {
            KmDebugMessage("[SELFTEST] Handled isr: ", context->vector, "\n");
            return *context;
        });

        __int<0x2>();

        InstallIsrHandler(0x2, old);
    }

    return isrs;
}

static void NormalizeProcessorState() {
    x64::Cr0 cr0 = x64::Cr0::load();
    cr0.set(x64::Cr0::WP | x64::Cr0::NE);
    x64::Cr0::store(cr0);

    kGsBase.store(0);
}

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

extern void operator delete(void *ptr, size_t size) noexcept {
    stdx::LockGuard _(gAllocatorLock);
    gAllocator->deallocate(ptr, size);
}

extern void *operator new(size_t size) {
    stdx::LockGuard _(gAllocatorLock);
    return gAllocator->allocate(size);
}

static uint32_t gSchedulerVector = 0;

void KmLaunchEx(boot::LaunchInfo launch) {
    NormalizeProcessorState();

    SetDebugLogLock(DebugLogLockType::eNone);

    InitBootTerminal(launch.framebuffers);

    std::optional<HypervisorInfo> hvInfo = KmGetHypervisorInfo();
    bool hasDebugPort = false;

    if (hvInfo.transform([](const HypervisorInfo& info) { return info.platformHasDebugPort(); }).value_or(false)) {
        hasDebugPort = KmTestDebugPort();
    }

    if (hasDebugPort) {
        gLogTargets.add(&gDebugPortLog);
    }

    ProcessorInfo processor = GetProcessorInfo();
    InitPortDelay(hvInfo);

    ComPortInfo com2Info = {
        .port = km::com::kComPort2,
        .divisor = km::com::kBaud9600,
    };

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
        .skipLoopbackTest = false,
    };

    SerialPortStatus com1Status = InitSerialPort(com1Info);
    SerialPortStatus com2Status = InitCanSerialPort(com2Info);

    LogSystemInfo(launch, hvInfo, processor, hasDebugPort, com1Status, com1Info, com2Status, com2Info);

    gBootGdt = GetBootGdt();
    SetupInitialGdt();

    km::IsrAllocator isrs = InitStage1Idt(SystemGdt::eLongModeCode);

    Stage1MemoryInfo stage1 = InitStage1Memory(launch, processor);
    Stage2MemoryInfo *stage2 = InitStage2Memory(launch, processor, stage1);
    gAllocator = &stage2->allocator;

    PlatformInfo platform = GetPlatformInfo(launch.smbios32Address, launch.smbios64Address, *stage2->memory);

    // On Oracle VirtualBox the COM1 port is functional but fails the loopback test.
    // If we are running on VirtualBox, retry the serial port initialization without the loopback test.
    if (com1Status == SerialPortStatus::eLoopbackTestFailed && platform.isOracleVirtualBox()) {
        UpdateSerialPort(com1Info);
        UpdateCanSerialPort(com2Info);
    }

    bool useX2Apic = kUseX2Apic && processor.has2xApic;

    auto [lapic, spuriousInt] = EnableBootApic(*stage2->memory, isrs, useX2Apic);

    acpi::AcpiTables rsdt = acpi::InitAcpi(launch.rsdpAddress, *stage2->memory);
    const acpi::Fadt *fadt = rsdt.fadt();
    InitCmos(fadt->century);

    uint32_t ioApicCount = rsdt.ioApicCount();
    KM_CHECK(ioApicCount > 0, "No IOAPICs found.");

    stdx::StaticVector<IoApic, 4> ioApics;

    for (uint32_t i = 0; i < ioApicCount; i++) {
        IoApic ioapic = rsdt.mapIoApic(*stage2->memory, 0);

        KmDebugMessage("[INIT] IOAPIC ", i, " ID: ", ioapic.id(), ", Version: ", ioapic.version(), "\n");
        KmDebugMessage("[INIT] ISR base: ", ioapic.isrBase(), ", Inputs: ", ioapic.inputCount(), "\n");

        ioApics.add(ioapic);
    }

    bool has8042 = rsdt.has8042Controller();

    hid::Ps2Controller ps2Controller;

    pci::ProbeConfigSpace();

    gSchedulerVector = isrs.allocateIsr();

    InitSmp(*stage2->memory, lapic.pointer(), rsdt, gSchedulerVector, spuriousInt);
    SetDebugLogLock(DebugLogLockType::eRecursiveSpinLock);

    // Setup gdt that contains a TSS for this core
    SetupApGdt();

    km::SetupUserMode(gAllocator);

    km::InitScheduler(isrs);

    uint8_t timer = isrs.allocateIsr();

    InstallIsrHandler(gSchedulerVector, [](km::IsrContext *ctx) -> km::IsrContext {
        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();
        return *ctx;
    });

    km::InitPit(100 * si::hertz, rsdt.madt(), ioApics.front(), lapic.pointer(), timer, [](IsrContext *ctx) -> km::IsrContext {
        IApic *apic = km::GetCpuLocalApic();
        apic->eoi();

        apic->sendIpi(apic::IcrDeliver::eOther, gSchedulerVector);
        return *ctx;
    });

    std::span init = GetInitProgram();

    auto process = LoadElf(init, 1, *stage2->memory, &stage2->allocator);
    KM_CHECK(process.has_value(), "Failed to load init.elf");

    DateTime time = ReadRtc();
    KmDebugMessage("[INIT] Current time: ", time.year, "-", time.month, "-", time.day, "T", time.hour, ":", time.minute, ":", time.second, "Z\n");

    km::EnterUserMode(process->main.regs);

    if (has8042) {
        hid::Ps2ControllerResult result = hid::EnablePs2Controller();
        KmDebugMessage("[INIT] PS/2 controller: ", result.status, "\n");
        if (result.status == hid::Ps2ControllerStatus::eOk) {
            ps2Controller = result.controller;

            KmDebugMessage("[INIT] PS/2 channel 1: ", present(ps2Controller.hasKeyboard()), "\n");
            KmDebugMessage("[INIT] PS/2 channel 2: ", present(ps2Controller.hasMouse()), "\n");
        }
    } else {
        KmDebugMessage("[INIT] No PS/2 controller found.\n");
    }

    if (ps2Controller.hasKeyboard()) {
        hid::Ps2Device keyboard = ps2Controller.keyboard();
        hid::Ps2Device mouse = ps2Controller.mouse();
        mouse.disable();
        keyboard.enable();

        while (true) {
            uint8_t scancode = ps2Controller.read();
            KmDebugMessage("[PS2] Keyboard scancode: ", Hex(scancode), "\n");
        }
    }

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
