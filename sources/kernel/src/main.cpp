// SPDX-License-Identifier: GPL-3.0-only

#include <numeric>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "apic.hpp"

#include "arch/cr0.hpp"
#include "arch/cr4.hpp"

#include "acpi/acpi.hpp"

#include <bezos/facility/device.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/debug.h>
#include <bezos/facility/mutex.h>

#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

#include "clock.hpp"
#include "cmos.hpp"
#include "debug/debug.hpp"
#include "delay.hpp"
#include "devices/ddi.hpp"
#include "devices/hid.hpp"
#include "devices/sysfs.hpp"
#include "display.hpp"
#include "drivers/block/ramblk.hpp"
#include "elf.hpp"
#include "fs2/path.hpp"
#include "fs2/utils.hpp"
#include "fs2/vfs.hpp"
#include "gdt.hpp"
#include "hid/hid.hpp"
#include "hid/ps2.hpp"
#include "hypervisor.hpp"
#include "isr/isr.hpp"
#include "isr/runtime.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "notify.hpp"
#include "panic.hpp"
#include "process/device.hpp"
#include "process/thread.hpp"
#include "processor.hpp"
#include "process/system.hpp"
#include "process/process.hpp"
#include "process/schedule.hpp"
#include "setup.hpp"
#include "smp.hpp"
#include "std/static_vector.hpp"
#include "syscall.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"
#include "thread.hpp"
#include "uart.hpp"
#include "smbios.hpp"
#include "pci/pci.hpp"

#include "timer/pit.hpp"
#include "timer/hpet.hpp"
#include "timer/apic_timer.hpp"
#include "timer/tsc_timer.hpp"

#include "memory/layout.hpp"

#include "allocator/tlsf.hpp"

#include "arch/intrin.hpp"

#include "kernel.hpp"

#include "user/sysapi.hpp"
#include "util/memory.hpp"

#include "fs2/vfs.hpp"
#include "fs2/tarfs.hpp"
#include "fs2/ramfs.hpp"
#include "xsave.hpp"

using namespace km;
using namespace stdx::literals;
using namespace std::string_view_literals;

namespace stdv = std::ranges::views;

static constexpr bool kUseX2Apic = true;
static constexpr bool kSelfTestIdt = true;
static constexpr bool kSelfTestApic = true;
static constexpr bool kEnableSmp = true;

static constexpr bool kEnableXSave = true;

// TODO: make this runtime configurable
static constexpr size_t kMaxMessageSize = 0x1000;

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

// static constexpr auto kRspStackSize = x64::kPageSize * 8;
static constexpr auto kTssStackSize = x64::kPageSize * 16;

enum {
    kIstTrap = 1,
    kIstTimer = 2,
    kIstNmi = 3,
    kIstIrq = 4,
};

void km::SetupInitialGdt(void) {
    InitGdt(gBootGdt.entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
}

void km::SetupApGdt(void) {
    // Save the gs/fs/kgsbase values, as setting the GDT will clear them
    CpuLocalRegisters tls = LoadTlsRegisters();
    SystemMemory *memory = GetSystemMemory();

    tlsTaskState = x64::TaskStateSegment {
        .ist1 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist2 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist3 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist4 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .iopbOffset = sizeof(x64::TaskStateSegment),
    };

    tlsSystemGdt = km::GetSystemGdt(&tlsTaskState);
    InitGdt(tlsSystemGdt->entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
    __ltr(SystemGdt::eTaskState0 * 0x8);

    StoreTlsRegisters(tls);
}

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
    stdx::Vector3<boot::MemoryRegion> memmap;

    MemoryMap(mem::IAllocator *allocator)
        : memmap(allocator)
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

static mem::TlsfAllocator *MakeEarlyAllocator(km::AddressMapping memory) {
    mem::TlsfAllocator allocator { (void*)memory.vaddr, memory.size };

    void *mem = allocator.allocateAligned(sizeof(mem::TlsfAllocator), alignof(mem::TlsfAllocator));

    return new (mem) mem::TlsfAllocator(std::move(allocator));
}

static MemoryMap* CreateEarlyAllocator(const boot::LaunchInfo& launch, mem::TlsfAllocator *allocator) {
    const auto& memmap = launch.memmap;
    km::AddressMapping bootMemory = launch.earlyMemory;
    km::MemoryRange range = bootMemory.physicalRange();

    MemoryMap *memory = allocator->construct<MemoryMap>(allocator);
    std::copy(memmap.begin(), memmap.end(), std::back_inserter(memory->memmap));

    memory->insert({ boot::MemoryRegion::eKernelRuntimeData, range });

    return memory;
}

static void MapDisplayRegions(PageTables& vmm, std::span<const boot::FrameBuffer> framebuffers, VirtualRange range) {
    uintptr_t framebufferBase = (uintptr_t)range.front;
    for (const boot::FrameBuffer& framebuffer : framebuffers) {
        // remap the framebuffer into its final location
        km::AddressMapping fb = { (void*)framebufferBase, framebuffer.paddr, framebuffer.size() };
        if (OsStatus status = vmm.map(fb, PageFlags::eData, MemoryType::eWriteCombine)) {
            KmDebugMessage("[INIT] Failed to map framebuffer: ", fb, " ", status, "\n");
            KM_PANIC("Failed to map framebuffer.");
        }

        framebufferBase += framebuffer.size();
    }
}

static void MapKernelRegions(PageTables &vmm, const KernelLayout& layout) {
    auto [kernelVirtualBase, kernelPhysicalBase, _] = layout.kernel;
    MapKernel(vmm, kernelPhysicalBase, kernelVirtualBase);
}

static void MapDataRegion(PageTables &vmm, km::AddressMapping mapping) {
    if (OsStatus status = vmm.map(mapping, PageFlags::eData)) {
        KmDebugMessage("[INIT] Failed to map data region: ", mapping, " ", status, "\n");
        KM_PANIC("Failed to map data region.");
    }
}

/// Everything in this structure is owned by the stage1 memory allocator.
/// Once that allocator is destroyed this memory is no longer mapped.
struct Stage1MemoryInfo {
    mem::TlsfAllocator *allocator;
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
    PageBuilder pm = PageBuilder { processor.maxpaddr, processor.maxvaddr, pat };

    WriteMtrrs(pm);

    mem::TlsfAllocator *allocator = MakeEarlyAllocator(launch.earlyMemory);
    MemoryMap *earlyMemory = CreateEarlyAllocator(launch, allocator);

    KernelLayout layout = BuildKernelLayout(
        processor.maxvaddr,
        launch.kernelPhysicalBase,
        launch.kernelVirtualBase,
        *earlyMemory,
        launch.framebuffers
    );

    WriteMemoryMap(earlyMemory->memmap);

    boot::FrameBuffer *framebuffers = CloneFrameBuffers(allocator, launch.framebuffers);

    static constexpr size_t kPtAllocSize = sm::kilobytes(256).bytes();
    void *pteMemory = allocator->allocateAligned(kPtAllocSize, x64::kPageSize);
    KM_CHECK(pteMemory != nullptr, "Failed to allocate memory for page tables.");

    size_t offset = (uintptr_t)pteMemory - (uintptr_t)launch.earlyMemory.vaddr;
    KM_CHECK(offset < launch.earlyMemory.size, "Page table memory is out of bounds.");

    AddressMapping pteMapping {
        .vaddr = pteMemory,
        .paddr = launch.earlyMemory.paddr + offset,
        .size = kPtAllocSize
    };

    PageTables vmm(&pm, pteMapping, PageFlags::eAll);

    // initialize our own page tables and remap everything into it
    MapKernelRegions(vmm, layout);

    // move our stack out of reclaimable memory
    MapDataRegion(vmm, stack);

    // map the early allocator region
    MapDataRegion(vmm, launch.earlyMemory);

    // map the non-pageable memory
    MapDataRegion(vmm, layout.committed);

    // remap framebuffers
    MapDisplayRegions(vmm, launch.framebuffers, layout.framebuffers);

    // once it is safe to remap the boot memory, do so
    UpdateRootPageTable(pm, vmm);

    // can't log anything here as we need to move the framebuffer first

    // Now update the terminal to use the new memory layout
    if (!launch.framebuffers.empty()) {
        gDirectTerminalLog.get()
            .display()
            .setAddress(layout.getFrameBuffer(launch.framebuffers, 0));
    }

    return Stage1MemoryInfo { allocator, earlyMemory, layout, std::span(framebuffers, launch.framebuffers.size()) };
}

struct Stage2MemoryInfo {
    SystemMemory *memory;
    KernelLayout layout;
};

static Stage2MemoryInfo *InitStage2Memory(const boot::LaunchInfo& launch, const km::ProcessorInfo& processor) {
    Stage1MemoryInfo stage1 = InitStage1Memory(launch, processor);
    km::AddressMapping stack = launch.stack;
    KernelLayout layout = stage1.layout;
    std::span<boot::FrameBuffer> framebuffers = stage1.framebuffers;
    MemoryMap *earlyMemory = stage1.earlyMemory;

    PageBuilder pm = PageBuilder { processor.maxpaddr, processor.maxvaddr, GetDefaultPatLayout() };

    InitGlobalAllocator((void*)layout.committed.vaddr, layout.committed.size);

    Stage2MemoryInfo *stage2 = new Stage2MemoryInfo();

    // TODO: rather than increasing this size we should use 1g pages for the big things
    // pcie ecam regions are the culprit here
    static constexpr size_t kPtAllocSize = sm::megabytes(3).bytes();
    void *pteMemory = aligned_alloc(x64::kPageSize, kPtAllocSize);
    KM_CHECK(pteMemory != nullptr, "Failed to allocate memory for page tables.");

    size_t offset = (uintptr_t)pteMemory - (uintptr_t)layout.committed.vaddr;
    KM_CHECK(offset < layout.committed.size, "Page table memory is out of bounds.");

    AddressMapping pteMapping {
        .vaddr = pteMemory,
        .paddr = layout.committed.paddr + offset,
        .size = kPtAllocSize
    };

    KmDebugMessage("[INIT] Page table memory: ", pteMapping, "\n");

    SystemMemory *memory = new SystemMemory(earlyMemory->memmap, stage1.layout.system, pm, pteMapping);
    KM_CHECK(memory != nullptr, "Failed to allocate memory for SystemMemory.");

    stage2->memory = memory;
    stage2->layout = layout;

    // carry forward the mappings made in stage 1
    // that are still required for kernel runtime
    memory->reserve(stack);
    memory->reserve(layout.committed);
    memory->reserve(layout.kernel);
    memory->reservePhysical({ 0zu, kLowMemory });

    // initialize our own page tables and remap everything into it
    MapKernelRegions(memory->systemTables(), layout);

    // map the stack again
    MapDataRegion(memory->systemTables(), stack);

    // carry forward the non-pageable memory
    MapDataRegion(memory->systemTables(), layout.committed);

    // remap framebuffers
    MapDisplayRegions(memory->systemTables(), framebuffers, layout.framebuffers);

    // once it is safe to remap the boot memory, do so
    UpdateRootPageTable(memory->getPageManager(), memory->systemTables());

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

static ApicInfo EnableBootApic(km::AddressSpace& memory, bool useX2Apic) {
    km::Apic apic = InitBspApic(memory, useX2Apic);

    // setup tls now that we have the lapic id

    km::InitCpuLocalRegion();
    km::RuntimeIsrManager::cpuInit();

    km::EnableCpuLocalIsrTable();

    SetDebugLogLock(DebugLogLockType::eSpinLock);
    km::InitKernelThread(apic);

    km::LocalIsrTable *ist = km::GetLocalIsrTable();

    const IsrEntry *spuriousInt = ist->allocate(SpuriousVector);
    uint8_t spuriousIdx = ist->index(spuriousInt);
    KmDebugMessage("[INIT] APIC ID: ", apic->id(), ", Version: ", apic->version(), ", Spurious vector: ", spuriousIdx, "\n");

    apic->setSpuriousVector(spuriousIdx);
    apic->enable();

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
        apic->sendIpi(apic::IcrDeliver::eSelf, testIdx);
        apic->selfIpi(testIdx);

        bool ok = ist->release(testInt, testIsr);
        KM_CHECK(ok, "Failed to release test ISR.");
    }

    return ApicInfo { apic, spuriousIdx };
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
    const ComPortInfo& com1Info) {
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
    KmDebugMessage("| /SYS/MB/CPU0  | XSAVE                | ", present(processor.xsave()), "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | TSC ratio            | ", processor.coreClock.tsc, "/", processor.coreClock.core, "\n");

    if (processor.hasNominalFrequency()) {
        KmDebugMessage("| /SYS/MB/CPU0  | Bus clock            | ", uint32_t(processor.busClock / si::hertz), "hz\n");
    } else {
        KmDebugMessage("| /SYS/MB/CPU0  | Bus clock            | Not available\n");
    }

    if (processor.hasBusFrequency()) {
        KmDebugMessage("| /SYS/MB/CPU0  | Base frequency       | ", uint16_t(processor.baseFrequency / si::megahertz), "mhz\n");
        KmDebugMessage("| /SYS/MB/CPU0  | Max frequency        | ", uint16_t(processor.maxFrequency / si::megahertz), "mhz\n");
        KmDebugMessage("| /SYS/MB/CPU0  | Bus frequency        | ", uint16_t(processor.busFrequency / si::megahertz), "mhz\n");
    } else {
        KmDebugMessage("| /SYS/MB/CPU0  | Base frequency       | Not reported via CPUID\n");
        KmDebugMessage("| /SYS/MB/CPU0  | Max frequency        | Not reported via CPUID\n");
        KmDebugMessage("| /SYS/MB/CPU0  | Bus frequency        | Not reported via CPUID\n");
    }

    KmDebugMessage("| /SYS/MB/CPU0  | Local APIC           | ", present(processor.lapic()), "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | 2x APIC              | ", present(processor.x2apic()), "\n");
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

    KmDebugMessage("| /BOOT         | Stack                | ", launch.stack, "\n");
    KmDebugMessage("| /BOOT         | Kernel virtual       | ", launch.kernelVirtualBase, "\n");
    KmDebugMessage("| /BOOT         | Kernel physical      | ", launch.kernelPhysicalBase, "\n");
    KmDebugMessage("| /BOOT         | Boot Allocator       | ", launch.earlyMemory, "\n");
    KmDebugMessage("| /BOOT         | INITRD               | ", launch.initrd, "\n");
    KmDebugMessage("| /BOOT         | HHDM offset          | ", Hex(launch.hhdmOffset).pad(16, '0'), "\n");
}

static void SetupInterruptStacks(uint16_t cs) {
    SystemMemory *memory = GetSystemMemory();
    gBootTss = x64::TaskStateSegment {
        .ist1 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist2 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist3 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist4 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,

        .iopbOffset = sizeof(x64::TaskStateSegment),
    };

    gBootGdt.setTss(&gBootTss);
    __ltr(SystemGdt::eTaskState0 * 0x8);

    for (uint8_t i = 0; i < isr::kExceptionCount; i++) {
        UpdateIdtEntry(i, cs, x64::Privilege::eSupervisor, kIstTrap);
    }

    for (auto i = isr::kExceptionCount; i < isr::kIsrCount; i++) {
        UpdateIdtEntry(i, cs, x64::Privilege::eSupervisor, kIstIrq);
    }

    UpdateIdtEntry(isr::NMI, cs, x64::Privilege::eSupervisor, kIstNmi);
    UpdateIdtEntry(isr::MCE, cs, x64::Privilege::eSupervisor, kIstNmi);
}

static void InitStage1Idt(uint16_t cs) {
    //
    // Disable the PIC before enabling interrupts otherwise we
    // get flooded by timer interrupts.
    //
    Disable8259Pic();

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
    IA32_EFER |= (1 << 11);

    IA32_GS_BASE = 0;
}

static vfs2::VfsRoot *gVfsRoot = nullptr;
static SystemObjects *gSystemObjects = nullptr;
static NotificationStream *gNotificationStream = nullptr;
static Scheduler *gScheduler = nullptr;
static SystemMemory *gMemory = nullptr;
static constinit Clock gClock{};

Scheduler *km::GetScheduler() {
    return gScheduler;
}

SystemMemory *km::GetSystemMemory() {
    return gMemory;
}

AddressSpace& km::GetProcessPageManager() {
    if (auto process = GetCurrentProcess()) {
        return *process->ptes;
    }

    KM_PANIC("Process page manager not available.");
}

PageTables& km::GetProcessPageTables() {
    return *GetProcessPageManager().tables();
}

static void CreateNotificationQueue() {
    static constexpr size_t kAqMemorySize = sm::kilobytes(128).bytes();
    SystemMemory *memory = GetSystemMemory();
    void *aqMemory = memory->allocate(kAqMemorySize + (x64::kPageSize * 2));
    // unmap first and last pages to catch buffer overruns
    memory->unmap(aqMemory, x64::kPageSize);
    memory->unmap((void*)((uintptr_t)aqMemory + kAqMemorySize + x64::kPageSize), x64::kPageSize);

    void *base = (void*)((uintptr_t)aqMemory + x64::kPageSize);
    InitAqAllocator(base, kAqMemorySize - x64::kPageSize);

    gNotificationStream = new NotificationStream();
}

static void MakeFolder(const vfs2::VfsPath& path) {
    sm::RcuSharedPtr<vfs2::INode> node = nullptr;
    if (OsStatus status = gVfsRoot->mkpath(path, &node)) {
        KmDebugMessage("[VFS] Failed to create path: '", path, "' ", status, "\n");
    }
}

template<typename... Args>
static void MakePath(Args&&... args) {
    MakeFolder(vfs2::BuildPath(std::forward<Args>(args)...));
}

static void MakeUser(vfs2::VfsStringView name) {
    MakePath("Users", name);
    MakePath("Users", name, "Programs");
    MakePath("Users", name, "Documents");
    MakePath("Users", name, "Options", "Local");
    MakePath("Users", name, "Options", "Domain");
}

static void MountRootVfs() {
    KmDebugMessage("[VFS] Initializing VFS.\n");
    gVfsRoot = new vfs2::VfsRoot();

    MakePath("System", "Options");
    MakePath("System", "NT");
    MakePath("System", "UNIX");
    MakePath("System", "Audit");

    MakePath("Devices");
    MakePath("Devices", "Terminal", "TTY0");

    MakePath("Processes");

    MakeUser("Guest");
    MakeUser("Operator");

    {
        sm::RcuSharedPtr<vfs2::INode> node = nullptr;
        vfs2::VfsPath path = vfs2::BuildPath("System", "Audit", "System.log");
        if (OsStatus status = gVfsRoot->create(path, &node)) {
            KmDebugMessage("[VFS] Failed to create ", path, ": ", status, "\n");
        }

        std::unique_ptr<vfs2::IFileHandle> log;
        if (OsStatus status = vfs2::OpenFileInterface(node, nullptr, 0, std::out_ptr(log))) {
            KmDebugMessage("[VFS] Failed to open log file: ", status, "\n");
        }
    }

    {
        sm::RcuSharedPtr<vfs2::INode> node = nullptr;
        vfs2::VfsPath path = vfs2::BuildPath("Users", "Guest", "motd.txt");
        if (OsStatus status = gVfsRoot->create(path, &node)) {
            KmDebugMessage("[VFS] Failed to create ", path, ": ", status, "\n");
        }

        std::unique_ptr<vfs2::IFileHandle> motd;
        if (OsStatus status = vfs2::OpenFileInterface(node, nullptr, 0, std::out_ptr(motd))) {
            KmDebugMessage("[VFS] Failed to open file: ", status, "\n");
        }

        char data[] = "Welcome.\n";
        vfs2::WriteRequest request {
            .begin = std::begin(data),
            .end = std::end(data) - 1,
        };
        vfs2::WriteResult result;

        if (OsStatus status = motd->write(request, &result)) {
            KmDebugMessage("[VFS] Failed to write file: ", status, "\n");
        }
    }
}

static void CreatePlatformVfsNodes(const km::SmBiosTables *smbios, const acpi::AcpiTables *acpi) {
    MakePath("Platform");

    {
        auto node = dev::SmBiosRoot::create(gVfsRoot->domain(), smbios);

        if (OsStatus status = gVfsRoot->mkdevice(vfs2::BuildPath("Platform", "SMBIOS"), node)) {
            KmDebugMessage("[VFS] Failed to create SMBIOS device: ", status, "\n");
        }
    }

    {
        auto node = dev::AcpiRoot::create(gVfsRoot->domain(), acpi);
        if (OsStatus status = gVfsRoot->mkdevice(vfs2::BuildPath("Platform", "ACPI"), node)) {
            KmDebugMessage("[VFS] Failed to create ACPI device: ", status, "\n");
        }
    }
}

static void MountInitArchive(MemoryRange initrd, AddressSpace& memory) {
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
    std::unique_ptr<vfs2::IFileHandle> init = nullptr;
    if (OsStatus status = gVfsRoot->open(vfs2::BuildPath("Init", "init.elf"), std::out_ptr(init))) {
        KmDebugMessage("[VFS] Failed to find '/Init/init.elf' ", status, "\n");
        KM_PANIC("Failed to open init process.");
    }

    return LoadElf(std::move(init), *gMemory, *gSystemObjects, launch);
}

static std::tuple<std::optional<HypervisorInfo>, bool> QueryHostHypervisor() {
    std::optional<HypervisorInfo> hvInfo = GetHypervisorInfo();
    bool hasDebugPort = false;

    if (hvInfo.transform([](const HypervisorInfo& info) { return info.platformHasDebugPort(); }).value_or(false)) {
        hasDebugPort = KmTestDebugPort();
    }

    if (hasDebugPort) {
        gLogTargets.add(&gDebugPortLog);
    }

    return std::make_tuple(hvInfo, hasDebugPort);
}

class SystemInvokeContext final : public vfs2::IInvokeContext {
    CallContext *mContext;
    km::SystemObjects *mSystem;

    OsThreadHandle thread() override {
        return mContext->thread()->publicId();
    }

    OsNodeHandle resolveNode(sm::RcuSharedPtr<vfs2::INode> node) override {
        if (OsNodeHandle handle = mSystem->getNodeId(node)) {
            return handle;
        }

        Node *result = nullptr;
        if (mSystem->createNode(mContext->process(), node, &result) != OsStatusSuccess) {
            return OS_HANDLE_INVALID;
        }

        return result->publicId();
    }

public:
    SystemInvokeContext(CallContext *context, km::SystemObjects *system)
        : mContext(context)
        , mSystem(system)
    { }
};

static void AddHandleSystemCalls() {
    AddSystemCall(eOsCallHandleWait, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userHandle = regs->arg0;
        uint64_t userTimeout = regs->arg1;
        Thread *thread = context->thread();

        BaseObject *object = gSystemObjects->getHandle(userHandle);
        if (object == nullptr) {
            return CallError(OsStatusInvalidHandle);
        }

        if (OsStatus status = thread->waitOnHandle(object, userTimeout)) {
            return CallError(status);
        }

        return CallOk(0zu);
    });
}

static void AddDebugSystemCalls() {
    AddSystemCall(eOsCallDebugMessage, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userMessage = regs->arg0;
        OsDebugMessageInfo messageInfo{};
        Process *process = context->process();
        Thread *thread = context->thread();

        if (OsStatus status = context->readObject(userMessage, &messageInfo)) {
            return CallError(status);
        }

        stdx::String message;
        if (OsStatus status = context->readString((uint64_t)messageInfo.Front, (uint64_t)messageInfo.Back, kMaxMessageSize, &message)) {
            return CallError(status);
        }

        LockDebugLog();

        for (const auto& segment : stdv::split(message, "\n"sv)) {
            std::string_view segmentView{segment};
            if (segmentView.empty()) {
                continue;
            }

            KmDebugMessageUnlocked("[DBG:", process->internalId(), ":", thread->internalId(), "] ", message, "\n");
        }

        UnlockDebugLog();

        return CallOk(0zu);
    });
}

static sys2::System *gSysSystem = nullptr;

static void InitSystem(km::LocalIsrTable *ist) {
    auto *memory = GetSystemMemory();
    gSysSystem = new sys2::System(&memory->pageTables(), &memory->pmmAllocator(), gVfsRoot);
    auto *scheduler = gSysSystem->scheduler();
    scheduler->initCpuSchedule(km::GetCurrentCoreId(), 128);
    sys2::InstallTimerIsr(ist);
}

static km::System GetSystem() {
    return km::System {
        .vfs = gVfsRoot,
        .objects = gSystemObjects,
        .memory = gMemory,
        .scheduler = gScheduler,
        .clock = &gClock,
        .sys = gSysSystem,
    };
}

static void AddDeviceSystemCalls() {
    AddSystemCall(eOsCallDeviceOpen, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::DeviceOpen(&system, context, regs);
    });

    AddSystemCall(eOsCallDeviceClose, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::DeviceClose(&system, context, regs);
    });

    AddSystemCall(eOsCallDeviceRead, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::DeviceRead(&system, context, regs);
    });

    AddSystemCall(eOsCallDeviceWrite, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::DeviceWrite(&system, context, regs);
    });

    AddSystemCall(eOsCallDeviceInvoke, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::DeviceInvoke(&system, context, regs);
    });

    AddSystemCall(eOsCallDeviceStat, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::DeviceStat(&system, context, regs);
    });
}

static void AddThreadSystemCalls() {
    AddSystemCall(eOsCallThreadSleep, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::ThreadSleep(&system, context, regs);
    });

    AddSystemCall(eOsCallThreadCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::ThreadCreate(&system, context, regs);
    });

    AddSystemCall(eOsCallThreadDestroy, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::ThreadDestroy(&system, context, regs);
    });
}

static void AddNodeSystemCalls() {
    AddSystemCall(eOsCallNodeOpen, [](CallContext *context, SystemCallRegisterSet *regs) {
        km::System system = GetSystem();
        return um::NodeOpen(&system, context, regs);
    });

    AddSystemCall(eOsCallNodeClose, [](CallContext *context, SystemCallRegisterSet *regs) {
        km::System system = GetSystem();
        return um::NodeClose(&system, context, regs);
    });

    AddSystemCall(eOsCallNodeQuery, [](CallContext *context, SystemCallRegisterSet *regs) {
        km::System system = GetSystem();
        return um::NodeQuery(&system, context, regs);
    });

    AddSystemCall(eOsCallNodeStat, [](CallContext *context, SystemCallRegisterSet *regs) {
        km::System system = GetSystem();
        return um::NodeStat(&system, context, regs);
    });
}

static void AddMutexSystemCalls() {
    AddSystemCall(eOsCallMutexCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::MutexCreate(&system, context, regs);
    });

    AddSystemCall(eOsCallMutexDestroy, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::MutexDestroy(&system, context, regs);
    });

    AddSystemCall(eOsCallMutexLock, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::MutexLock(&system, context, regs);
    });

    AddSystemCall(eOsCallMutexUnlock, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::MutexUnlock(&system, context, regs);
    });
}

static void AddProcessSystemCalls() {
    AddSystemCall(eOsCallProcessCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        System system = GetSystem();
        return um::ProcessCreate(&system, context, regs);
    });

    AddSystemCall(eOsCallProcessDestroy, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        System system = GetSystem();
        return um::ProcessDestroy(&system, context, regs);
    });

    AddSystemCall(eOsCallProcessCurrent, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        System system = GetSystem();
        return um::ProcessCurrent(&system, context, regs);
    });

    AddSystemCall(eOsCallProcessStat, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        System system = GetSystem();
        return um::ProcessStat(&system, context, regs);
    });
}

static void AddVmemSystemCalls() {
    AddSystemCall(eOsCallVmemCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        System system = GetSystem();
        return um::VmemCreate(&system, context, regs);
    });

    AddSystemCall(eOsCallVmemRelease, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        System system = GetSystem();
        return um::VmemDestroy(&system, context, regs);
    });
}

static void AddClockSystemCalls() {
    AddSystemCall(eOsCallClockStat, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        auto system = GetSystem();
        return um::ClockStat(&system, context, regs);
    });

    AddSystemCall(eOsCallClockGetTime, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        auto system = GetSystem();
        return um::ClockTime(&system, context, regs);
    });

    AddSystemCall(eOsCallClockTicks, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        auto system = GetSystem();
        return um::ClockTicks(&system, context, regs);
    });
}

static void EnableUmip(bool enable) {
    if (enable) {
        x64::Cr4 cr4 = x64::Cr4::load();
        cr4.set(x64::Cr4::UMIP);
        x64::Cr4::store(cr4);
    }
}

static void StartupSmp(const acpi::AcpiTables& rsdt, bool umip, km::ApicTimer *apicTimer) {
    //
    // Create the scheduler and system objects before we startup SMP so that
    // the AP cores have a scheduler to attach to.
    //
    gScheduler = new Scheduler();
    gSystemObjects = new SystemObjects(gMemory, gVfsRoot);

    std::atomic_flag launchScheduler = ATOMIC_FLAG_INIT;

    if constexpr (kEnableSmp) {
        //
        // We provide an atomic flag to the AP cores that we use to signal when the
        // scheduler is ready to be used. The scheduler requires the system to switch
        // to using cpu local isr tables, which must happen after smp startup.
        //
        InitSmp(*GetSystemMemory(), GetCpuLocalApic(), rsdt, [&launchScheduler, umip, apicTimer](LocalIsrTable *ist, IApic *apic) {
            while (!launchScheduler.test()) {
                _mm_pause();
            }

            EnableUmip(umip);

            if constexpr (um::kUseNewSystem) {
                auto *scheduler = gSysSystem->scheduler();
                scheduler->initCpuSchedule(km::GetCurrentCoreId(), 128);
                sys2::EnterScheduler(ist, gSysSystem->getCpuSchedule(km::GetCurrentCoreId()), apicTimer);
            } else {
                km::ScheduleWork(ist, apic);
            }
        });
    }

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
    Process *process = GetCurrentProcess();
    Thread *thread = gSystemObjects->createThread(std::move(name), process);

    km::AddressMapping mapping = gMemory->allocateStack(kKernelStackSize);

    thread->userStack = mapping;
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

static OsStatus NotificationWork(void *) {
    while (true) {
        gNotificationStream->processAll();
    }

    return OsStatusSuccess;
}

static OsStatus KernelMasterTask() {
    KmDebugMessageUnlocked("[INIT] Kernel master task.\n");

    KmHalt();

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
        km::YieldCurrentThread();
    }

    return OsStatusSuccess;
}

static constexpr bool kEnableMouse = false;

static void ConfigurePs2Controller(const acpi::AcpiTables& rsdt, IoApicSet& ioApicSet, const IApic *apic, LocalIsrTable *ist) {
    static hid::Ps2Controller ps2Controller;

    bool has8042 = rsdt.has8042Controller();

    hid::InitPs2HidStream(gNotificationStream);

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

        if constexpr (kEnableMouse) {
            ps2Controller.setMouseSampleRate(10);
            ps2Controller.setMouseResolution(1);
            hid::InstallPs2MouseIsr(ioApicSet, ps2Controller, apic, ist);
        }

        hid::InstallPs2KeyboardIsr(ioApicSet, ps2Controller, apic, ist);

        mouse.disable();
        keyboard.enable();
        ps2Controller.enableIrqs(true, kEnableMouse);
    }

    vfs2::VfsPath hidPs2DevicePath{OS_DEVICE_PS2_KEYBOARD};

    {
        sm::RcuSharedPtr<vfs2::INode> node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(hidPs2DevicePath.parent(), &node)) {
            KmDebugMessage("[VFS] Failed to create ", hidPs2DevicePath.parent(), " folder: ", status, "\n");
            KM_PANIC("Failed to create keyboar device folder.");
        }
    }

    {
        auto device = sm::rcuMakeShared<dev::HidKeyboardDevice>(gVfsRoot->domain());
        if (OsStatus status = gVfsRoot->mkdevice(hidPs2DevicePath, device)) {
            KmDebugMessage("[VFS] Failed to create ", hidPs2DevicePath, " device: ", status, "\n");
            KM_PANIC("Failed to create keyboard device.");
        }

        gNotificationStream->subscribe(hid::GetHidPs2Topic(), device.get());
    }
}

static void CreateDisplayDevice() {
    vfs2::VfsPath ddiPath{OS_DEVICE_DDI_RAMFB};

    {
        sm::RcuSharedPtr<vfs2::INode> node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(ddiPath.parent(), &node)) {
            KmDebugMessage("[VFS] Failed to create ", ddiPath.parent(), " folder: ", status, "\n");
            KM_PANIC("Failed to create display device folder.");
        }
    }

    {
        auto device = sm::rcuMakeShared<dev::DisplayDevice>(gVfsRoot->domain(), gDirectTerminalLog.get().display());
        if (OsStatus status = gVfsRoot->mkdevice(ddiPath, device)) {
            KmDebugMessage("[VFS] Failed to create ", ddiPath, " device: ", status, "\n");
            KM_PANIC("Failed to create display device.");
        }
    }
}

[[noreturn]]
static void LaunchKernelProcess(LocalIsrTable *table, IApic *apic, km::ApicTimer *apicTimer) {
    if constexpr (um::kUseNewSystem) {
        sys2::ProcessCreateInfo createInfo {
            .name = "SYSTEM",
            .state = eOsProcessSupervisor,
        };
        std::unique_ptr<sys2::ProcessHandle> system;
        OsStatus status = sys2::SysCreateRootProcess(gSysSystem, createInfo, std::out_ptr(system));
        if (status != OsStatusSuccess) {
            KmDebugMessage("[INIT] Failed to create SYSTEM process: ", status, "\n");
            KM_PANIC("Failed to create SYSTEM process.");
        }

        KmDebugMessage("[INIT] Create master task.\n");

        km::AddressMapping mapping = gMemory->allocateStack(kKernelStackSize);
        OsThreadCreateInfo threadInfo {
            .Name = "SYSTEM MASTER TASK",
            .CpuState = {
                .rbp = (uintptr_t)((uintptr_t)mapping.vaddr + kKernelStackSize),
                .rsp = (uintptr_t)((uintptr_t)mapping.vaddr + kKernelStackSize),
                .rip = (uintptr_t)&KernelMasterTask,
            },
            .Flags = eOsThreadRunning,
        };

        OsThreadHandle thread = OS_HANDLE_INVALID;
        sys2::InvokeContext invoke { gSysSystem, system->getProcess() };
        status = sys2::SysCreateThread(&invoke, threadInfo, &thread);
        if (status != OsStatusSuccess) {
            KmDebugMessage("[INIT] Failed to create SYSTEM thread: ", status, "\n");
            KM_PANIC("Failed to create SYSTEM thread.");
        }

        KmDebugMessage("[INIT] Create master thread.\n");

        sys2::EnterScheduler(table, gSysSystem->getCpuSchedule(km::GetCurrentCoreId()), apicTimer);
    } else {
        MemoryRange pteMemory = gMemory->pmmAllocate(256);

        ProcessCreateInfo createInfo {
            .parent = nullptr,
            .privilege = x64::Privilege::eSupervisor,
        };

        Process *process = nullptr;
        OsStatus status = gSystemObjects->createProcess("SYSTEM", pteMemory, createInfo, &process);
        if (status != OsStatusSuccess) {
            KmDebugMessage("[INIT] Failed to create SYSTEM process: ", status, "\n");
            KM_PANIC("Failed to create SYSTEM process.");
        }

        Thread *thread = gSystemObjects->createThread("SYSTEM MASTER TASK", process);

        km::AddressMapping mapping = gMemory->allocateStack(kKernelStackSize);

        thread->userStack = mapping;
        thread->state = km::IsrContext {
            .rbp = (uintptr_t)((uintptr_t)mapping.vaddr + kKernelStackSize),
            .rip = (uintptr_t)&KernelMasterTask,
            .cs = SystemGdt::eLongModeCode * 0x8,
            .rflags = 0x202,
            .rsp = (uintptr_t)((uintptr_t)mapping.vaddr + kKernelStackSize),
            .ss = SystemGdt::eLongModeData * 0x8,
        };

        gScheduler->addWorkItem(thread);
        ScheduleWork(table, apic);
    }
}

static void InitVfs() {
    MountRootVfs();
    MountVolatileFolder();
}

static void CreateVfsDevices(const km::SmBiosTables *smbios, const acpi::AcpiTables *acpi, MemoryRange initrd) {
    CreatePlatformVfsNodes(smbios, acpi);
    MountInitArchive(initrd, GetSystemMemory()->pageTables());
}

static void InitUserApi() {
    AddHandleSystemCalls();
    AddDebugSystemCalls();
    AddDeviceSystemCalls();
    AddNodeSystemCalls();
    AddThreadSystemCalls();
    AddVmemSystemCalls();
    AddMutexSystemCalls();
    AddProcessSystemCalls();
    AddClockSystemCalls();
}

void LaunchKernel(boot::LaunchInfo launch) {
    NormalizeProcessorState();
    SetDebugLogLock(DebugLogLockType::eNone);
    InitBootTerminal(launch.framebuffers);
    auto [hvInfo, hasDebugPort] = QueryHostHypervisor();

#if 0
    TestCanvas(gDirectTerminalLog.get().display());
    KmHalt();
#endif

    ProcessorInfo processor = GetProcessorInfo();
    EnableUmip(processor.umip());

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

    if (OsStatus status = debug::InitDebugStream(com2Info)) {
        KmDebugMessage("[INIT] Failed to initialize debug port: ", status, "\n");
    }

    LogSystemInfo(launch, hvInfo, processor, hasDebugPort, com1Status, com1Info);

    gBootGdt = GetBootGdt();
    SetupInitialGdt();

    XSaveConfig xsaveConfig {
        .target = kEnableXSave ? SaveMode::eXSave : SaveMode::eFxSave,
        .features = XSaveMask(x64::FPU, x64::SSE, x64::AVX) | x64::kSaveAvx512,
        .cpuInfo = &processor,
    };

    InitFpuSave(xsaveConfig);

    KmDebugMessage("[INIT] CR0: ", x64::Cr0::load(), "\n");
    KmDebugMessage("[INIT] CR4: ", x64::Cr4::load(), "\n");

    //
    // Once we have the initial gdt setup we create the global allocator.
    // The IDT depends on the allocator to create its global ISR table.
    //
    Stage2MemoryInfo *stage2 = InitStage2Memory(launch, processor);
    gMemory = stage2->memory;

    InitStage1Idt(SystemGdt::eLongModeCode);
    EnableInterrupts();

    InitVfs();

    SmBiosLoadOptions smbiosOptions {
        .smbios32Address = launch.smbios32Address,
        .smbios64Address = launch.smbios64Address,
    };

    km::SmBiosTables smbios{};

    if (OsStatus status = FindSmbiosTables(smbiosOptions, gMemory->pageTables(), &smbios)) {
        KmDebugMessage("[INIT] Failed to find SMBIOS tables: ", status, "\n");
    }

    //
    // On Oracle VirtualBox the COM1 port is functional but fails the loopback test.
    // If we are running on VirtualBox, retry the serial port initialization without the loopback test.
    //
    if (com1Status == SerialPortStatus::eLoopbackTestFailed && smbios.isOracleVirtualBox()) {
        UpdateSerialPort(com1Info);
    }

    bool useX2Apic = kUseX2Apic && processor.x2apic();

    auto [lapic, spuriousInt] = EnableBootApic(gMemory->pageTables(), useX2Apic);

    acpi::AcpiTables rsdt = acpi::InitAcpi(launch.rsdpAddress, gMemory->pageTables());
    const acpi::Fadt *fadt = rsdt.fadt();
    InitCmos(fadt->century);

    uint32_t ioApicCount = rsdt.ioApicCount();
    KM_CHECK(ioApicCount > 0, "No IOAPICs found.");
    IoApicSet ioApicSet{ rsdt.madt(), gMemory->pageTables() };

    std::unique_ptr<pci::IConfigSpace> config{pci::InitConfigSpace(rsdt.mcfg(), gMemory->pageTables())};
    if (!config) {
        KM_PANIC("Failed to initialize PCI config space.");
    }

    pci::ProbeConfigSpace(config.get(), rsdt.mcfg());

    // TODO: this wastes some physical memory
    {
        static constexpr size_t kSchedulerMemorySize = 0x10000;
        void *mapping = gMemory->allocate(kSchedulerMemorySize + (x64::kPageSize * 2));
        // unmap the first and last page to guard against buffer overruns
        gMemory->unmap(mapping, x64::kPageSize);
        gMemory->unmap((void*)((uintptr_t)mapping + kSchedulerMemorySize + x64::kPageSize), x64::kPageSize);

        void *ptr = (void*)((uintptr_t)mapping + x64::kPageSize);

        InitSchedulerMemory(ptr, kSchedulerMemorySize);
    }

    km::LocalIsrTable *ist = GetLocalIsrTable();

    if constexpr (um::kUseNewSystem) {
        InitSystem(ist);
    } else {
        InstallSchedulerIsr(ist);
    }

    const IsrEntry *timerInt = ist->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();
        return *ctx;
    });
    uint8_t timerIdx = ist->index(timerInt);
    KmDebugMessage("[INIT] Timer ISR: ", timerIdx, "\n");

    stdx::StaticVector<km::ITickSource*, 4> tickSources;

    km::IntervalTimer pit = km::InitPit(100 * si::hertz, ioApicSet, lapic.pointer(), timerIdx);

    tickSources.add(&pit);
    km::ITickSource *tickSource = &pit;

    km::HighPrecisionTimer hpet;
    if (OsStatus status = km::InitHpet(rsdt, gMemory->pageTables(), &hpet)) {
        KmDebugMessage("[INIT] Failed to initialize HPET: ", status, "\n");
    } else {
        hpet.enable(true);
        tickSources.add(&hpet);
        tickSource = &hpet;

        KmDebugMessage("|---- HPET -----------------\n");
        KmDebugMessage("| Property      | Value\n");
        KmDebugMessage("|---------------+-----------\n");
        KmDebugMessage("| Vendor        | ", hpet.vendor(), "\n");
        KmDebugMessage("| Timer count   | ", hpet.timerCount(), "\n");
        KmDebugMessage("| Clock         | ", hpet.refclk(), "\n");
        KmDebugMessage("| Counter size  | ", hpet.counterSize() == hpet::Width::DWORD ? "32"_sv : "64"_sv, "\n");
        KmDebugMessage("| Revision      | ", hpet.revision(), "\n");
        KmDebugMessage("| Ticks         | ", hpet.ticks(), "\n");

        auto comparators = hpet.comparators();
        for (size_t i = 0; i < comparators.size(); i++) {
            const auto& comparator = comparators[i];
            auto config = comparator.config();
            KmDebugMessage("| Comparator    | ", i, "\n");
            KmDebugMessage("|  - Route Mask | ", comparator.routeMask(), "\n");
            KmDebugMessage("|  - Counter    | ", comparator.counter(), "\n");
            KmDebugMessage("|  - FSB        | ", comparator.fsbIntDelivery(), "\n");
            KmDebugMessage("|  - Periodic   | ", comparator.periodicSupport(), "\n");
            KmDebugMessage("|  - Width      | ", comparator.width() == hpet::Width::DWORD ? "32"_sv : "64"_sv, "\n");
            KmDebugMessage("|  - Enabled    | ", config.enable, "\n");
            KmDebugMessage("|  - Period     | ", config.period, "\n");
            KmDebugMessage("|  - Mode       | ", config.mode == hpet::Width::DWORD ? "32"_sv : "64"_sv, "\n");
            KmDebugMessage("|  - Trigger    | ", config.trigger == apic::Trigger::eLevel ? "Level"_sv : "Edge"_sv, "\n");
            KmDebugMessage("|  - Route      | ", config.ioApicRoute, "\n");
        }
    }

    KmDebugMessage("[INIT] Training APIC timer.\n");
    km::ITickSource *clockTicker = tickSource;
    ApicTimer apicTimer;
    InvariantTsc tsc;

    if (OsStatus status = km::TrainApicTimer(lapic.pointer(), tickSources.back(), &apicTimer)) {
        KmDebugMessage("[INIT] Failed to train APIC timer: ", status, "\n");
    } else {
        KmDebugMessage("[INIT] APIC timer frequency: ", apicTimer.refclk(), "\n");
        tickSources.add(&apicTimer);
    }

    if (processor.invariantTsc) {
        if (OsStatus status = km::TrainInvariantTsc(tickSource, &tsc)) {
            KmDebugMessage("[INIT] Failed to train invariant TSC: ", status, "\n");
        } else {
            KmDebugMessage("[INIT] Invariant TSC frequency: ", tsc.frequency(), "\n");
            tickSources.add(&tsc);
            clockTicker = &tsc;
        }
    }

    StartupSmp(rsdt, processor.umip(), &apicTimer);

    DateTime time = ReadCmosClock();
    KmDebugMessage("[INIT] Current time: ", time.year, "-", time.month, "-", time.day, "T", time.hour, ":", time.minute, ":", time.second, "Z\n");

    gClock = Clock { time, clockTicker };

    CreateVfsDevices(&smbios, &rsdt, launch.initrd);
    InitUserApi();

    CreateNotificationQueue();

    ConfigurePs2Controller(rsdt, ioApicSet, lapic.pointer(), ist);
    CreateDisplayDevice();

    LaunchKernelProcess(ist, lapic.pointer(), &apicTimer);

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
