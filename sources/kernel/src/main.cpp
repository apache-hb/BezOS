// SPDX-License-Identifier: GPL-3.0-only

#include <numeric>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "apic.hpp"

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
#include "devices/sysfs.hpp"
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
#include "notify.hpp"
#include "panic.hpp"
#include "pit.hpp"
#include "process/process.hpp"
#include "processor.hpp"
#include "process/schedule.hpp"
#include "setup.hpp"
#include "smp.hpp"
#include "std/static_vector.hpp"
#include "syscall.hpp"
#include "thread.hpp"
#include "uart.hpp"
#include "smbios.hpp"
#include "pci/pci.hpp"

#include "memory/layout.hpp"

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
static constexpr bool kEnableSmp = true;

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;
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

static constinit SerialPort gCanBusPort;
static constinit CanBus gCanBus{nullptr};

static constexpr size_t kTssStackSize = x64::kPageSize * 8;

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
    SystemMemory *memory = GetSystemMemory();

    tlsTaskState = x64::TaskStateSegment {
        .ist1 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist2 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist3 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
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
        vmm.map(fb, PageFlags::eData, MemoryType::eWriteCombine);

        framebufferBase += framebuffer.size();
    }
}

static void MapKernelRegions(PageTables &vmm, const KernelLayout& layout) {
    auto [kernelVirtualBase, kernelPhysicalBase, _] = layout.kernel;
    MapKernel(vmm, kernelPhysicalBase, kernelVirtualBase);
}

static void MapDataRegion(PageTables &vmm, km::AddressMapping mapping) {
    vmm.map(mapping, PageFlags::eData);
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

    static constexpr size_t kPtAllocSize = sm::megabytes(1).bytes();
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
    UpdateRootPageTable(memory->getPager(), memory->systemTables());

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
    KmDebugMessage("| /BOOT         | Boot Allocator       | ", launch.earlyMemory, "\n");
    KmDebugMessage("| /BOOT         | INITRD               | ", launch.initrd, "\n");
    KmDebugMessage("| /BOOT         | HHDM offset          | ", Hex(launch.hhdmOffset).pad(16, '0'), "\n");
}

static void SetupInterruptStacks(uint16_t cs) {
    SystemMemory *memory = GetSystemMemory();
    gBootTss = x64::TaskStateSegment{
        .ist1 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist2 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
        .ist3 = (uintptr_t)memory->allocateStack(kTssStackSize).vaddr + kTssStackSize,
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

AddressSpaceAllocator& km::GetProcessPageManager() {
    if (auto process = GetCurrentProcess()) {
        return process->ptes;
    }

    return gMemory->pageTables();
}

PageTables& km::GetProcessPageTables() {
    return GetProcessPageManager().ptes();
}

static void CreateNotificationQueue() {
    static constexpr size_t kAqMemorySize = sm::kilobytes(128).bytes();
    void *memory = aligned_alloc(x64::kPageSize, kAqMemorySize);
    InitAqAllocator(memory, kAqMemorySize);

    gNotificationStream = new NotificationStream();
}

static void MakeFolder(const vfs2::VfsPath& path) {
    vfs2::IVfsNode *node = nullptr;
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

    MakePath("Devices");
    MakePath("Processes");

    MakeUser("Guest");
    MakeUser("Operator");

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
            .end = std::end(data) - 1,
        };
        vfs2::WriteResult result;

        if (OsStatus status = motd->write(request, &result)) {
            KmDebugMessage("[VFS] Failed to write file: ", status, "\n");
        }
    }
}

static void CreatePlatformVfsNodes(const km::SmBiosTables *smbios) {
    MakePath("Platform");

    {
        vfs2::IVfsNode *node = new dev::SmBiosTables(smbios);

        if (OsStatus status = gVfsRoot->mkdevice(vfs2::BuildPath("Platform", "SMBIOS"), node)) {
            KmDebugMessage("[VFS] Failed to create SMBIOS device: ", status, "\n");
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

static OsStatus UserReadPath(CallContext *context, const char *front, const char *back, vfs2::VfsPath *path) {
    vfs2::VfsString text;
    if (OsStatus status = context->readString((uint64_t)front, (uint64_t)back, kMaxPathSize, &text)) {
        return status;
    }

    if (!vfs2::VerifyPathText(text)) {
        return OsStatusInvalidPath;
    }

    *path = vfs2::VfsPath{std::move(text)};
    return OsStatusSuccess;
}

static void AddDebugSystemCalls() {
    AddSystemCall(eOsCallDebugLog, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t front = regs->arg0;
        uint64_t back = regs->arg1;

        stdx::String message;
        if (OsStatus status = context->readString(front, back, kMaxMessageSize, &message)) {
            return CallError(status);
        }

        KmDebugMessage(message);

        return CallOk(0zu);
    });
}

static void AddVfsFileSystemCalls() {
    AddSystemCall(eOsCallFileOpen, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userCreateInfo = regs->arg0;
        OsFileCreateInfo createInfo{};
        if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
            return CallError(status);
        }

        vfs2::VfsPath path;
        if (OsStatus status = UserReadPath(context, createInfo.PathFront, createInfo.PathBack, &path)) {
            return CallError(status);
        }

        std::unique_ptr<vfs2::IVfsNodeHandle> node = nullptr;
        if (OsStatus status = gVfsRoot->open(path, std::out_ptr(node))) {
            return CallError(status);
        }

        vfs2::IVfsNodeHandle *ptr = context->process()->addFile(std::move(node));

        return CallOk(ptr);
    });

    AddSystemCall(eOsCallFileRead, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t nodeId = regs->arg0;
        uint64_t front = regs->arg1;
        uint64_t back = regs->arg2;

        if (!context->isMapped(front, back, PageFlags::eUser | PageFlags::eWrite)) {
            return CallError(OsStatusInvalidInput);
        }

        auto file = context->process()->findFile((const vfs2::IVfsNodeHandle*)nodeId);
        if (file == nullptr) {
            return CallError(OsStatusInvalidHandle);
        }

        vfs2::ReadRequest request {
            .begin = (void*)front,
            .end = (void*)back,
        };
        vfs2::ReadResult result{};
        if (OsStatus status = file->read(request, &result)) {
            return CallError(status);
        }

        return CallOk(result.read);
    });
}

static void AddVfsFolderSystemCalls() {
    AddSystemCall(eOsCallFolderIterateCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userCreateInfo = regs->arg0;

        OsFolderIterateCreateInfo createInfo{};
        if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
            return CallError(status);
        }

        vfs2::VfsPath path;
        if (OsStatus status = UserReadPath(context, createInfo.PathFront, createInfo.PathBack, &path)) {
            return CallError(status);
        }

        std::unique_ptr<vfs2::IVfsNodeHandle> node = nullptr;
        if (OsStatus status = gVfsRoot->opendir(path, std::out_ptr(node))) {
            return CallError(status);
        }

        vfs2::IVfsNodeHandle *ptr = context->process()->addFile(std::move(node));

        return CallOk(ptr);
    });

    AddSystemCall(eOsCallFolderIterateNext, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userHandle = regs->arg0;
        uint64_t userEntry = regs->arg1;

        vfs2::IVfsNodeHandle *handle = context->process()->findFile((const vfs2::IVfsNodeHandle*)userHandle);
        if (handle == nullptr) {
            return CallError(OsStatusInvalidHandle);
        }

        OsFolderEntry header{};
        if (OsStatus status = context->readObject(userEntry, &header)) {
            return CallError(status);
        }

        OsFolderEntryNameSize size = header.NameSize;
        if (size > kMaxPathSize) {
            return CallError(OsStatusInvalidInput);
        }

        vfs2::VfsString name;
        if (OsStatus status = handle->next(&name)) {
            return CallError(status);
        }

        OsFolderEntryNameSize limit = std::min<OsFolderEntryNameSize>(size, name.count());

        OsFolderEntry result { .NameSize = limit, };

        //
        // First write back the header.
        //
        if (OsStatus status = context->writeObject(userEntry, result)) {
            return CallError(status);
        }

        //
        // Then write the name itself.
        //
        if (OsStatus status = context->writeRange(userEntry + sizeof(OsFolderEntry), name.begin(), name.data() + limit)) {
            return CallError(status);
        }

        return CallOk(0zu);
    });
}

static void AddVfsSystemCalls() {
    AddVfsFileSystemCalls();
    AddVfsFolderSystemCalls();
}

static void AddDeviceSystemCalls() {
    AddSystemCall(eOsCallDeviceOpen, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userCreateInfo = regs->arg0;
        OsDeviceCreateInfo createInfo{};
        if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
            return CallError(status);
        }

        vfs2::VfsPath path;
        if (OsStatus status = UserReadPath(context, createInfo.NameFront, createInfo.NameBack, &path)) {
            return CallError(status);
        }

        sm::uuid uuid = createInfo.InterfaceGuid;
        km::Process *process = context->process().get();

        std::unique_ptr<vfs2::IVfsNodeHandle> handle = nullptr;
        if (OsStatus status = gVfsRoot->device(path, uuid, std::out_ptr(handle))) {
            return CallError(status);
        }

        vfs2::IVfsNodeHandle *ptr = process->addFile(std::move(handle));

        return CallOk(ptr);
    });

    AddSystemCall(eOsCallDeviceClose, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userDevice = regs->arg0;
        km::Process *process = context->process().get();
        if (OsStatus status = process->closeFile((vfs2::IVfsNodeHandle*)userDevice)) {
            return CallError(status);
        }

        return CallOk(0zu);
    });

    AddSystemCall(eOsCallDeviceRead, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userHandle = regs->arg0;
        uint64_t userRequest = regs->arg1;

        OsDeviceReadRequest request{};
        if (OsStatus status = context->readObject(userRequest, &request)) {
            return CallError(status);
        }

        if (!context->isMapped((uint64_t)request.BufferFront, (uint64_t)request.BufferBack, PageFlags::eUser | PageFlags::eWrite)) {
            return CallError(OsStatusInvalidInput);
        }

        vfs2::IVfsNodeHandle *handle = context->process()->findFile((const vfs2::IVfsNodeHandle*)userHandle);
        if (handle == nullptr) {
            return CallError(OsStatusInvalidHandle);
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

    AddSystemCall(eOsCallDeviceCall, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userHandle = regs->arg0;
        uint64_t userFunction = regs->arg1;
        uint64_t userData = regs->arg2;
        uint64_t userSize = regs->arg3;

        km::Process *process = context->process().get();

        if (!context->isMapped((uint64_t)userData, (uint64_t)userData + userSize, PageFlags::eUserData)) {
            return CallError(OsStatusInvalidInput);
        }

        vfs2::IVfsNodeHandle *handle = process->findFile((const vfs2::IVfsNodeHandle*)userHandle);
        if (handle == nullptr) {
            return CallError(OsStatusInvalidHandle);
        }

        if (OsStatus status = handle->call(userFunction, (void*)userData, userSize)) {
            return CallError(status);
        }

        return CallOk(0zu);
    });
}

static void AddThreadSystemCalls() {
    AddSystemCall(eOsCallThreadSleep, [](CallContext *, SystemCallRegisterSet *) -> OsCallResult {
        km::YieldCurrentThread();
        return CallOk(0zu);
    });

    AddSystemCall(eOsCallThreadCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userCreateInfo = regs->arg0;

        OsThreadCreateInfo createInfo{};
        if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
            return CallError(status);
        }

        if ((createInfo.StackSize % x64::kPageSize != 0) || (createInfo.StackSize < x64::kPageSize)) {
            return CallError(OsStatusInvalidInput);
        }

        stdx::String name;
        if (OsStatus status = context->readString((uint64_t)createInfo.NameFront, (uint64_t)createInfo.NameBack, kMaxPathSize, &name)) {
            return CallError(status);
        }

        sm::RcuSharedPtr<Process> process;
        if (createInfo.Process != OS_HANDLE_INVALID) {
            process = gSystemObjects->getProcess(ProcessId(createInfo.Process & 0x00FF'FFFF'FFFF'FFFF));
        } else {
            process = GetCurrentProcess();
        }

        if (process == nullptr) {
            return CallError(OsStatusNotFound);
        }

        stdx::String stackName = name + " STACK";
        sm::RcuSharedPtr<Thread> thread = gSystemObjects->createThread(std::move(name), process);

        PageFlags flags = PageFlags::eUser | PageFlags::eData;
        MemoryType type = MemoryType::eWriteBack;

        AddressMapping mapping{};
        SystemMemory *memory = GetSystemMemory();
        OsStatus status = AllocateMemory(memory->pmmAllocator(), &process->ptes, createInfo.StackSize / x64::kPageSize, &mapping);
        if (status != OsStatusSuccess) {
            return CallError(status);
        }

        sm::RcuSharedPtr<AddressSpace> stackSpace = gSystemObjects->createAddressSpace(std::move(stackName), mapping, flags, type, process);
        thread->stack = stackSpace;
        thread->state = km::IsrContext {
            .rbp = (uintptr_t)(thread->stack.get() + createInfo.StackSize),
            .rip = (uintptr_t)createInfo.EntryPoint,
            .cs = (SystemGdt::eLongModeUserCode * 0x8) | 0b11,
            .rflags = 0x202,
            .rsp = (uintptr_t)(thread->stack.get() + createInfo.StackSize),
            .ss = (SystemGdt::eLongModeUserData * 0x8) | 0b11,
        };
        return CallError(OsStatusInvalidFunction);
    });
}

static void AddMutexSystemCalls() {
    AddSystemCall(eOsCallMutexCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userCreateInfo = regs->arg0;

        OsMutexCreateInfo createInfo{};
        if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
            return CallError(status);
        }

        stdx::String name;
        if (OsStatus status = context->readString((uint64_t)createInfo.NameFront, (uint64_t)createInfo.NameBack, kMaxPathSize, &name)) {
            return CallError(status);
        }

        sm::RcuSharedPtr<Mutex> mutex = gSystemObjects->createMutex(std::move(name));
        return CallOk(mutex->id());
    });

    //
    // Clang can't analyze the control flow here because its all very non-local.
    //
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wthread-safety-analysis"

    AddSystemCall(eOsCallMutexLock, [](CallContext*, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t id = regs->arg0;
        sm::RcuSharedPtr<Mutex> mutex = gSystemObjects->getMutex(MutexId(id & 0x00FF'FFFF'FFFF'FFFF));
        if (mutex == nullptr) {
            return CallError(OsStatusNotFound);
        }

        //
        // Try a few times to acquire the lock before putting the thread to sleep.
        //
        for (int i = 0; i < 10; i++) {
            if (mutex->lock.try_lock()) {
                return CallOk(0zu);
            }
        }

        while (!mutex->lock.try_lock()) {
            km::YieldCurrentThread();
        }

        return CallOk(0zu);
    });

    AddSystemCall(eOsCallMutexUnlock, [](CallContext*, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t id = regs->arg0;
        sm::RcuSharedPtr<Mutex> mutex = gSystemObjects->getMutex(MutexId(id & 0x00FF'FFFF'FFFF'FFFF));
        if (mutex == nullptr) {
            return CallError(OsStatusNotFound);
        }

        mutex->lock.unlock();
        return CallOk(0zu);
    });

#pragma clang diagnostic pop
}

static void AddProcessSystemCalls() {
    AddSystemCall(eOsCallProcessCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userCreateInfo = regs->arg0;
        OsProcessCreateInfo createInfo{};
        if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
            return CallError(status);
        }

        vfs2::VfsPath path;
        if (OsStatus status = UserReadPath(context, createInfo.Executable.Front, createInfo.Executable.Back, &path)) {
            return CallError(status);
        }

        // stdx::String args;
        // if (OsStatus status = context->readString((uint64_t)createInfo.ArgumentsBegin, (uint64_t)createInfo.ArgumentsEnd, kMaxPathSize, &args)) {
        //     return CallError(status);
        // }

        std::unique_ptr<vfs2::IVfsNodeHandle> node = nullptr;
        if (OsStatus status = gVfsRoot->open(path, std::out_ptr(node))) {
            return CallError(status);
        }

        ProcessLaunch launch{};
        if (OsStatus status = LoadElf(std::move(node), *gMemory, *gSystemObjects, &launch)) {
            return CallError(status);
        }

        GetScheduler()->addWorkItem(launch.main);

        return CallOk(launch.process->id());
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
    AddSystemCall(eOsCallAddressSpaceCreate, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userCreateInfo = regs->arg0;
        OsAddressSpaceCreateInfo createInfo{};
        if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
            return CallError(status);
        }

        PageFlags flags = PageFlags::eUser;
        if (OsStatus status = GetMemoryAccess(createInfo.Access, &flags)) {
            return CallError(status);
        }

        sm::RcuSharedPtr<Process> process;
        if (createInfo.Process != OS_HANDLE_INVALID) {
            process = gSystemObjects->getProcess(ProcessId(createInfo.Process & 0x00FF'FFFF'FFFF'FFFF));
        } else {
            process = GetCurrentProcess();
        }

        if (process == nullptr) {
            return CallError(OsStatusNotFound);
        }

        stdx::String name;
        if (OsStatus status = context->readString((uint64_t)createInfo.NameFront, (uint64_t)createInfo.NameBack, kMaxPathSize, &name)) {
            return CallError(status);
        }

        AddressMapping mapping{};
        SystemMemory *memory = GetSystemMemory();
        OsStatus status = AllocateMemory(memory->pmmAllocator(), &process->ptes, createInfo.Size / x64::kPageSize, &mapping);
        if (status != OsStatusSuccess) {
            return CallError(status);
        }

        sm::RcuSharedPtr<AddressSpace> addressSpace = gSystemObjects->createAddressSpace(std::move(name), mapping, flags, MemoryType::eWriteBack, process);
        return CallOk(addressSpace->id());
    });

    AddSystemCall(eOsCallAddressSpaceStat, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t id = regs->arg0;
        uint64_t userStat = regs->arg1;

        sm::RcuSharedPtr<AddressSpace> space = gSystemObjects->getAddressSpace(AddressSpaceId(id & 0x00FF'FFFF'FFFF'FFFF));
        km::AddressMapping mapping = space->mapping;

        OsAddressSpaceInfo stat {
            .Base = (OsAnyPointer)mapping.vaddr,
            .Size = mapping.size,

            .Access = MakeMemoryAccess(space->flags),
        };

        if (OsStatus status = context->writeObject(userStat, stat)) {
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
    sm::RcuSharedPtr<Process> process = GetCurrentProcess();
    sm::RcuSharedPtr<Thread> thread = gSystemObjects->createThread(std::move(name), process);

    PageFlags flags = PageFlags::eData;
    MemoryType type = MemoryType::eWriteBack;
    km::AddressMapping mapping = gMemory->allocateStack(kKernelStackSize);
    sm::RcuSharedPtr<AddressSpace> stackSpace = gSystemObjects->createAddressSpace(std::move(stackName), mapping, flags, type, process);

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
    MemoryRange pteMemory = gMemory->pmmAllocate(256);

    sm::RcuSharedPtr<Process> process = gSystemObjects->createProcess("SYSTEM", x64::Privilege::eSupervisor, &gMemory->pageTables(), pteMemory);
    sm::RcuSharedPtr<Thread> thread = gSystemObjects->createThread("MASTER", process);

    PageFlags flags = PageFlags::eData;
    MemoryType type = MemoryType::eWriteBack;
    km::AddressMapping mapping = gMemory->allocateStack(kKernelStackSize);
    sm::RcuSharedPtr<AddressSpace> stackSpace = gSystemObjects->createAddressSpace("MASTER STACK", mapping, flags, type, process);

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
    Stage2MemoryInfo *stage2 = InitStage2Memory(launch, processor);
    gMemory = stage2->memory;

    //
    // I need to disable the PIC before enabling interrupts otherwise I
    // get flooded by timer interrupts.
    //
    Disable8259Pic();

    InitStage1Idt(SystemGdt::eLongModeCode);
    EnableInterrupts();

    SmBiosLoadOptions smbiosOptions {
        .smbios32Address = launch.smbios32Address,
        .smbios64Address = launch.smbios64Address,
    };

    km::SmBiosTables smbios{};

    if (OsStatus status = FindSmbiosTables(smbiosOptions, *gMemory, &smbios)) {
        KmDebugMessage("[INIT] Failed to find SMBIOS tables: ", status, "\n");
    }

    //
    // On Oracle VirtualBox the COM1 port is functional but fails the loopback test.
    // If we are running on VirtualBox, retry the serial port initialization without the loopback test.
    //
    if (com1Status == SerialPortStatus::eLoopbackTestFailed && smbios.isOracleVirtualBox()) {
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

    static constexpr size_t kSchedulerMemorySize = 0x10000;
    InitSchedulerMemory(aligned_alloc(x64::kPageSize, kSchedulerMemorySize), kSchedulerMemorySize);

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
    CreatePlatformVfsNodes(&smbios);
    MountVolatileFolder();
    MountInitArchive(launch.initrd, *stage2->memory);

    AddDebugSystemCalls();
    AddVfsSystemCalls();
    AddDeviceSystemCalls();
    AddThreadSystemCalls();
    AddVmemSystemCalls();
    AddMutexSystemCalls();
    AddProcessSystemCalls();

    CreateNotificationQueue();

    ConfigurePs2Controller(rsdt, ioApicSet, lapic.pointer(), ist);
    CreateDisplayDevice();

    LaunchKernelProcess(ist, lapic.pointer());

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
