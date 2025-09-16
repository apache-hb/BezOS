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
#include "delay.hpp"

#include "debug/debug.hpp"

#include "devices/ddi.hpp"
#include "devices/hid.hpp"
#include "devices/sysfs.hpp"
#include "drivers/block/ramblk.hpp"
#include "fs/vfs.hpp"
#include "fs/path.hpp"
#include "fs/utils.hpp"
#include "fs/tarfs.hpp"
#include "fs/ramfs.hpp"

#include "elf.hpp"
#include "gdt.hpp"
#include "hypervisor.hpp"

#include "hid/hid.hpp"
#include "hid/ps2.hpp"
#include "isr/isr.hpp"
#include "isr/runtime.hpp"

#include "logger/logger.hpp"
#include "logger/categories.hpp"
#include "logger/serial_appender.hpp"
#include "logger/vga_appender.hpp"
#include "logger/e9_appender.hpp"

#include "memory.hpp"
#include "memory/stack_mapping.hpp"
#include "notify.hpp"
#include "panic.hpp"
#include "processor.hpp"
#include "setup.hpp"
#include "smp.hpp"

#include "std/static_vector.hpp"

#include "syscall.hpp"
#include "system/invoke.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"

#include "task/runtime.hpp"
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
#include "common/util/defer.hpp"

#include "util/memory.hpp"

#include "xsave.hpp"

using namespace km;
using namespace stdx::literals;
using namespace std::string_view_literals;

namespace stdv = std::ranges::views;

static constexpr bool kUseX2Apic = true;
static constexpr bool kSelfTestIdt = true;
static constexpr bool kSelfTestApic = true;
static constexpr bool kEnableSmp = true;
static constexpr bool kEnableMouse = false;
static constexpr bool kEnableXSave = true;

// TODO: make this runtime configurable
static constexpr size_t kMaxMessageSize = 0x1000;
static constexpr size_t kKernelStackSize = 0x4000;
static constexpr auto kTssStackSize = x64::kPageSize * 16;
static constexpr std::chrono::milliseconds kDefaultTimeSlice = std::chrono::milliseconds(25);

constinit static km::SerialAppender gSerialAppender;
constinit static km::VgaAppender gVgaAppender;
constinit static km::E9Appender gDebugPortAppender;

constinit km::CpuLocal<SystemGdt> km::tlsSystemGdt;
constinit km::CpuLocal<x64::TaskStateSegment> km::tlsTaskState;

static constinit x64::TaskStateSegment gBootTss{};
static constinit SystemGdt gBootGdt{};

enum {
    kIstTrap = 1,
    kIstTimer = 2,
    kIstNmi = 3,
    kIstIrq = 4,
};

void km::setupInitialGdt(void) {
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

    tlsSystemGdt = km::getSystemGdt(&tlsTaskState);
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
        InitLog.fatalf("Failed to reserve memory for committed region ", sm::bytes(kCommittedRegionSize), ".");
        KM_PANIC("Failed to reserve memory for committed region.");
    }

    VirtualRange committedVirtualRange = builder.reserve(kCommittedRegionSize);
    km::AddressMapping committed = { committedVirtualRange.front, committedPhysicalMemory.front, kCommittedRegionSize };

    VirtualRange framebuffers = builder.reserve(framebuffersSize);

    AddressMapping kernel = { kernelVirtualBase, kernelPhysicalBase, GetKernelSize() };

    InitLog.dbgf("Kernel layout:");
    InitLog.dbgf("Data         : ", data);
    InitLog.dbgf("Committed    : ", committed);
    InitLog.dbgf("Framebuffers : ", framebuffers);
    InitLog.dbgf("Kernel       : ", kernel);

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
            InitLog.fatalf("Failed to map framebuffer: ", fb, " ", OsStatusId(status));
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
        InitLog.fatalf("Failed to map data region: ", mapping, " ", OsStatusId(status));
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

    static constexpr size_t kPtAllocSize = sm::kilobytes(512).bytes();
    void *pteMemory = allocator->allocateAligned(kPtAllocSize, x64::kPageSize);
    KM_CHECK(pteMemory != nullptr, "Failed to allocate memory for page tables.");

    size_t offset = (uintptr_t)pteMemory - (uintptr_t)launch.earlyMemory.vaddr;
    KM_CHECK(offset < launch.earlyMemory.size, "Page table memory is out of bounds.");

    AddressMapping pteMapping {
        .vaddr = pteMemory,
        .paddr = launch.earlyMemory.paddr + offset,
        .size = kPtAllocSize
    };

    PageTables vmm;
    if (OsStatus status = km::PageTables::create(&pm, pteMapping, PageFlags::eAll, &vmm)) {
        InitLog.fatalf("Failed to create page tables: ", OsStatusId(status));
        KM_PANIC("Failed to create page tables.");
    }

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
        gVgaAppender.setAddress(layout.getFrameBuffer(launch.framebuffers, 0));
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

    InitLog.dbgf("Page table memory: ", pteMapping);

    SystemMemory *memory = new SystemMemory;
    KM_CHECK(memory != nullptr, "Failed to allocate memory for SystemMemory.");

    OsStatus status = km::SystemMemory::create(earlyMemory->memmap, stage1.layout.system.cast<sm::VirtualAddress>(), pm, pteMapping, memory);
    KM_CHECK(status == OsStatusSuccess, "Failed to create SystemMemory.");

    stage2->memory = memory;
    stage2->layout = layout;

    // carry forward the mappings made in stage 1
    // that are still required for kernel runtime
    memory->reserve(stack);
    // memory->reserve(layout.committed);
    // memory->reserve(layout.kernel);
    // memory->reservePhysical({ 0zu, kLowMemory });

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

static km::IsrContext SpuriousVector(km::IsrContext *ctx) noexcept [[clang::reentrant]] {
    IsrLog.warnf("Spurious interrupt: ", ctx->vector);
    km::IApic *pic = km::GetCpuLocalApic();
    pic->eoi();
    return *ctx;
}

static km::Apic enableBootApic(km::AddressSpace& memory, bool useX2Apic) {
    km::Apic apic = InitBspApic(memory, useX2Apic);

    // setup tls now that we have the lapic id

    km::InitCpuLocalRegion();
    km::RuntimeIsrManager::cpuInit();

    km::InitKernelThread(apic);

    km::SharedIsrTable *sharedIst = km::GetSharedIsrTable();

    sharedIst->install(isr::kSpuriousVector, SpuriousVector);
    InitLog.infof("APIC ID: ", apic->id(), ", Version: ", apic->version(), ", Spurious vector: ", isr::kSpuriousVector);

    apic->setSpuriousVector(isr::kSpuriousVector);
    apic->enable();

    if (kSelfTestApic) {
        km::LocalIsrTable *ist = km::GetLocalIsrTable();
        IsrCallback testIsr = [](km::IsrContext *ctx) noexcept [[clang::reentrant]] -> km::IsrContext {
            TestLog.dbgf("Handled isr: ", ctx->vector);
            km::IApic *pic = km::GetCpuLocalApic();
            pic->eoi();
            return *ctx;
        };

        const IsrEntry *testInt = ist->allocate(testIsr);
        uint8_t testIdx = ist->index(testInt);

        TestLog.dbgf("Testing self-IPI ", testIdx);

        //
        // Test that both self-IPI methods work.
        //
        apic->sendIpi(apic::IcrDeliver::eSelf, testIdx);
        apic->selfIpi(testIdx);

        bool ok = ist->release(testInt, testIsr);
        KM_CHECK(ok, "Failed to release test ISR.");
    }

    return apic;
}

static void initBootTerminal(std::span<const boot::FrameBuffer> framebuffers) {
    if (framebuffers.empty()) return;

    if (km::VgaAppender::create(framebuffers.front(), (void*)framebuffers.front().vaddr, &gVgaAppender) != OsStatusSuccess) {
        return;
    }

    LogQueue::addGlobalAppender(&gVgaAppender);
}

static void updateSerialPort(ComPortInfo info) {
    info.skipLoopbackTest = true;
    if (OpenSerialResult com = OpenSerial(info); com.status == SerialPortStatus::eOk) {
        SerialAppender::create(com.port, &gSerialAppender);
        LogQueue::addGlobalAppender(&gSerialAppender);
    }
}

static SerialPortStatus initSerialPort(ComPortInfo info) {
    if (OpenSerialResult com = OpenSerial(info)) {
        return com.status;
    } else {
        SerialAppender::create(com.port, &gSerialAppender);
        LogQueue::addGlobalAppender(&gSerialAppender);
        return SerialPortStatus::eOk;
    }
}

static void initPortDelay(const std::optional<HypervisorInfo>& hvInfo) {
    bool isKvm = hvInfo.transform([](const HypervisorInfo& hv) { return hv.isKvm(); }).value_or(false);
    x64::PortDelay delay = isKvm ? x64::PortDelay::eNone : x64::PortDelay::ePostCode;

    KmSetPortDelayMethod(delay);
}

static void displaySystemInfo(
    const boot::LaunchInfo& launch,
    std::optional<HypervisorInfo> hvInfo,
    ProcessorInfo processor,
    bool hasDebugPort,
    SerialPortStatus com1Status,
    const ComPortInfo& com1Info
) {

    InitLog.infof("System report.");
    InitLog.println("| Component     | Property             | Status");
    InitLog.println("|---------------+----------------------+-------");

    if (hvInfo.has_value()) {
        InitLog.println("| /SYS/HV       | Hypervisor           | ", hvInfo->name);
        InitLog.println("| /SYS/HV       | Max HvCPUID leaf     | ", Hex(hvInfo->maxleaf).pad(8, '0'));
        InitLog.println("| /SYS/HV       | e9 debug port        | ", enabled(hasDebugPort));
    } else {
        InitLog.println("| /SYS/HV       | Hypervisor           | Not present");
        InitLog.println("| /SYS/HV       | Max HvCPUID leaf     | Not applicable");
        InitLog.println("| /SYS/HV       | e9 debug port        | Not applicable");
    }

    InitLog.println("| /SYS/MB/CPU0  | Vendor               | ", processor.vendor);
    InitLog.println("| /SYS/MB/CPU0  | Model name           | ", processor.brand);
    InitLog.println("| /SYS/MB/CPU0  | Max CPUID leaf       | ", Hex(processor.maxleaf).pad(8, '0'));
    InitLog.println("| /SYS/MB/CPU0  | Max physical address | ", processor.maxpaddr);
    InitLog.println("| /SYS/MB/CPU0  | Max virtual address  | ", processor.maxvaddr);
    InitLog.println("| /SYS/MB/CPU0  | XSAVE                | ", present(processor.xsave()));
    InitLog.println("| /SYS/MB/CPU0  | TSC ratio            | ", processor.coreClock.tsc, "/", processor.coreClock.core);

    if (processor.hasNominalFrequency()) {
        InitLog.println("| /SYS/MB/CPU0  | Bus clock            | ", uint32_t(processor.busClock / si::hertz), "hz");
    } else {
        InitLog.println("| /SYS/MB/CPU0  | Bus clock            | Not available");
    }

    if (processor.hasBusFrequency()) {
        InitLog.println("| /SYS/MB/CPU0  | Base frequency       | ", uint16_t(processor.baseFrequency / si::megahertz), "mhz");
        InitLog.println("| /SYS/MB/CPU0  | Max frequency        | ", uint16_t(processor.maxFrequency / si::megahertz), "mhz");
        InitLog.println("| /SYS/MB/CPU0  | Bus frequency        | ", uint16_t(processor.busFrequency / si::megahertz), "mhz");
    } else {
        InitLog.println("| /SYS/MB/CPU0  | Base frequency       | Not reported via CPUID");
        InitLog.println("| /SYS/MB/CPU0  | Max frequency        | Not reported via CPUID");
        InitLog.println("| /SYS/MB/CPU0  | Bus frequency        | Not reported via CPUID");
    }

    InitLog.println("| /SYS/MB/CPU0  | Local APIC           | ", present(processor.lapic()));
    InitLog.println("| /SYS/MB/CPU0  | 2x APIC              | ", present(processor.x2apic()));
    InitLog.println("| /SYS/MB/CPU0  | Invariant TSC        | ", present(processor.invariantTsc));
    InitLog.println("| /SYS/MB/CPU0  | SMBIOS 32 address    | ", launch.smbios32Address);
    InitLog.println("| /SYS/MB/CPU0  | SMBIOS 64 address    | ", launch.smbios64Address);

    for (size_t i = 0; i < launch.framebuffers.size(); i++) {
        const boot::FrameBuffer& display = launch.framebuffers[i];
        InitLog.println("| /SYS/VIDEO", i, "   | Display resolution   | ", display.width, "x", display.height, "x", display.bpp);
        InitLog.println("| /SYS/VIDEO", i, "   | Framebuffer size     | ", sm::bytes(display.size()));
        InitLog.println("| /SYS/VIDEO", i, "   | Framebuffer address  | ", display.vaddr);
        InitLog.println("| /SYS/VIDEO", i, "   | Display pitch        | ", display.pitch);
        InitLog.println("| /SYS/VIDEO", i, "   | EDID                 | ", display.edid);
        InitLog.println("| /SYS/VIDEO", i, "   | Red channel          | (mask=", display.redMaskSize, ",shift=", display.redMaskShift, ")");
        InitLog.println("| /SYS/VIDEO", i, "   | Green channel        | (mask=", display.greenMaskSize, ",shift=", display.greenMaskShift, ")");
        InitLog.println("| /SYS/VIDEO", i, "   | Blue channel         | (mask=", display.blueMaskSize, ",shift=", display.blueMaskShift, ")");
    }

    InitLog.println("| /SYS/MB/COM1  | Status               | ", com1Status);
    InitLog.println("| /SYS/MB/COM1  | Port                 | ", Hex(com1Info.port));
    InitLog.println("| /SYS/MB/COM1  | Baud rate            | ", km::com::kBaudRate / com1Info.divisor);

    InitLog.println("| /BOOT         | Stack                | ", launch.stack);
    InitLog.println("| /BOOT         | Kernel virtual       | ", launch.kernelVirtualBase);
    InitLog.println("| /BOOT         | Kernel physical      | ", launch.kernelPhysicalBase);
    InitLog.println("| /BOOT         | Boot Allocator       | ", launch.earlyMemory);
    InitLog.println("| /BOOT         | INITRD               | ", launch.initrd);
    InitLog.println("| /BOOT         | HHDM offset          | ", Hex(launch.hhdmOffset).pad(16, '0'));
}

static void setupInterruptStacks(uint16_t cs) {
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

static void initStage1Idt(uint16_t cs) {
    //
    // Disable the PIC before enabling interrupts otherwise we
    // get flooded by timer interrupts.
    //
    Disable8259Pic();

    InitInterrupts(cs);
    InstallExceptionHandlers(GetSharedIsrTable());
    setupInterruptStacks(cs);

    if (kSelfTestIdt) {
        km::LocalIsrTable *ist = GetLocalIsrTable();
        IsrCallback old = ist->install(64, [](km::IsrContext *context) noexcept [[clang::reentrant]] -> km::IsrContext {
            TestLog.dbgf("Handled isr: ", context->vector);
            return *context;
        });

        __int<64>();

        ist->install(64, old);
    }
}

static void normalizeProcessorState() {
    x64::Cr0 cr0 = x64::Cr0::of(x64::Cr0::PG | x64::Cr0::WP | x64::Cr0::NE | x64::Cr0::ET | x64::Cr0::PE);
    x64::Cr0::store(cr0);

    x64::Cr4 cr4 = x64::Cr4::of(x64::Cr4::PAE);
    x64::Cr4::store(cr4);

    // Enable NXE bit
    IA32_EFER |= (1 << 11);

    IA32_GS_BASE = 0;
}

static vfs::VfsRoot *gVfsRoot = nullptr;
static NotificationStream *gNotificationStream = nullptr;
static SystemMemory *gMemory = nullptr;
static constinit Clock gClock{};

SystemMemory *km::GetSystemMemory() {
    return gMemory;
}

sys::AddressSpaceManager& km::GetProcessPageManager() {
    if (auto process = sys::GetCurrentProcess()) {
        return *process->getAddressSpaceManager();
    }

    KM_PANIC("Process page manager not available.");
}

PageTables& km::GetProcessPageTables() {
    return GetProcessPageManager().getPageTables();
}

static void createNotificationQueue() {
    gNotificationStream = new NotificationStream();
}

static void MakeFolder(const vfs::VfsPath& path) {
    sm::RcuSharedPtr<vfs::INode> node = nullptr;
    if (OsStatus status = gVfsRoot->mkpath(path, &node)) {
        VfsLog.warnf("Failed to create path: '", path, "' ", OsStatusId(status));
    }
}

template<typename... Args>
static void MakePath(Args&&... args) {
    MakeFolder(vfs::BuildPath(std::forward<Args>(args)...));
}

static void MakeUser(vfs::VfsStringView name) {
    MakePath("Users", name);
    MakePath("Users", name, "Programs");
    MakePath("Users", name, "Documents");
    MakePath("Users", name, "Options", "Local");
    MakePath("Users", name, "Options", "Domain");
}

static void MountRootVfs() {
    VfsLog.infof("Initializing VFS.");
    gVfsRoot = new vfs::VfsRoot();

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
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        vfs::VfsPath path = vfs::BuildPath("System", "Audit", "System.log");
        if (OsStatus status = gVfsRoot->create(path, &node)) {
            VfsLog.warnf("Failed to create ", path, ": ", OsStatusId(status));
        }

        std::unique_ptr<vfs::IFileHandle> log;
        if (OsStatus status = vfs::OpenFileInterface(node, nullptr, 0, std::out_ptr(log))) {
            VfsLog.warnf("Failed to open log file: ", OsStatusId(status));
        }
    }

    {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        vfs::VfsPath path = vfs::BuildPath("Users", "Guest", "motd.txt");
        if (OsStatus status = gVfsRoot->create(path, &node)) {
            VfsLog.warnf("Failed to create ", path, ": ", OsStatusId(status));
        }

        std::unique_ptr<vfs::IFileHandle> motd;
        if (OsStatus status = vfs::OpenFileInterface(node, nullptr, 0, std::out_ptr(motd))) {
            VfsLog.warnf("Failed to open file: ", OsStatusId(status));
        }

        char data[] = "Welcome.\n";
        vfs::WriteRequest request {
            .begin = std::begin(data),
            .end = std::end(data) - 1,
        };
        vfs::WriteResult result;

        if (OsStatus status = motd->write(request, &result)) {
            VfsLog.warnf("Failed to write file: ", OsStatusId(status));
        }
    }
}

static void CreatePlatformVfsNodes(const km::SmBiosTables *smbios, const acpi::AcpiTables *acpi) {
    MakePath("Platform");

    {
        auto node = dev::SmBiosRoot::create(gVfsRoot->domain(), smbios);

        if (OsStatus status = gVfsRoot->mkdevice(vfs::BuildPath("Platform", "SMBIOS"), node)) {
            VfsLog.warnf("Failed to create SMBIOS device: ", OsStatusId(status));
        }
    }

    {
        auto node = dev::AcpiRoot::create(gVfsRoot->domain(), acpi);
        if (OsStatus status = gVfsRoot->mkdevice(vfs::BuildPath("Platform", "ACPI"), node)) {
            VfsLog.warnf("Failed to create ACPI device: ", OsStatusId(status));
        }
    }
}

static void MountInitArchive(MemoryRangeEx initrd, AddressSpace& memory) {
    VfsLog.infof("Mounting '/Init'");

    //
    // If the initrd is empty then it wasn't found in the boot parameters.
    // Not much we can do without an initrd.
    //
    if (initrd.isEmpty()) {
        VfsLog.fatalf("No initrd found.");
        KM_PANIC("No initrd found.");
    }

    TlsfAllocation initrdMemory;
    if (OsStatus status = memory.map(km::alignedOut(initrd, x64::kPageSize), PageFlags::eRead, MemoryType::eWriteBack, &initrdMemory)) {
        VfsLog.fatalf("Failed to map initrd: ", OsStatusId(status));
        KM_PANIC("Failed to map initrd.");
    }

    sm::SharedPtr<MemoryBlk> block = new MemoryBlk{std::bit_cast<std::byte*>(initrdMemory.address()), initrd.size()};

    vfs::IVfsMount *mount = nullptr;
    if (OsStatus status = gVfsRoot->addMountWithParams(&vfs::TarFs::instance(), vfs::BuildPath("Init"), &mount, block)) {
        VfsLog.fatalf("Failed to mount initrd: ", OsStatusId(status));
        KM_PANIC("Failed to mount initrd.");
    }
}

static void MountVolatileFolder() {
    VfsLog.infof("Mounting '/Volatile'");

    vfs::IVfsMount *mount = nullptr;
    if (OsStatus status = gVfsRoot->addMount(&vfs::RamFs::instance(), vfs::BuildPath("Volatile"), &mount)) {
        VfsLog.fatalf("Failed to mount '/Volatile' ", OsStatusId(status));
        KM_PANIC("Failed to mount volatile folder.");
    }
}

static OsStatus launchInitProcess(sys::InvokeContext *invoke, OsProcessHandle *process) {
    OsDeviceHandle device = OS_HANDLE_INVALID;
    OsThreadHandle thread = OS_HANDLE_INVALID;
    sys::DeviceOpenInfo createInfo {
        .path = vfs::BuildPath("Init", "init.elf"),
        .flags = eOsDeviceOpenExisting,
        .interface = kOsFileGuid,
    };

    if (OsStatus status = sys::SysDeviceOpen(invoke, createInfo, &device)) {
        VfsLog.fatalf("Failed to create device ", createInfo.path, ": ", OsStatusId(status));
        return status;
    }

    defer { sys::SysDeviceClose(invoke, device); };

    if (OsStatus status = km::LoadElf2(invoke, device, process, &thread)) {
        SysLog.fatalf("Failed to load init process: ", OsStatusId(status));
        return status;
    }

    if (OsStatus status = sys::SysThreadSuspend(invoke, thread, false)) {
        SysLog.fatalf("Failed to resume init thread: ", OsStatusId(status));
        return status;
    }

    return OsStatusSuccess;
}

static std::tuple<std::optional<HypervisorInfo>, bool> queryHostHypervisor() {
    std::optional<HypervisorInfo> hvInfo = GetHypervisorInfo();
    bool hasDebugPort = false;

    if (hvInfo.transform([](const HypervisorInfo& info) { return info.platformHasDebugPort(); }).value_or(false)) {
        hasDebugPort = E9Appender::isAvailable();
    }

    if (hasDebugPort) {
        LogQueue::addGlobalAppender(&gDebugPortAppender);
    }

    return std::make_tuple(hvInfo, hasDebugPort);
}

static void AddDebugSystemCalls() {
    AddSystemCall(eOsCallDebugMessage, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        uint64_t userMessage = regs->arg0;
        OsDebugMessageInfo messageInfo{};
        auto process = sys::GetCurrentProcess();
        auto thread = sys::GetCurrentThread();

        if (OsStatus status = context->readObject(userMessage, &messageInfo)) {
            return CallError(status);
        }

        stdx::String message;
        if (OsStatus status = context->readString((uint64_t)messageInfo.Front, (uint64_t)messageInfo.Back, kMaxMessageSize, &message)) {
            return CallError(status);
        }

        for (const auto& segment : stdv::split(message, "\n"sv)) {
            std::string_view segmentView{segment};
            if (segmentView.empty()) {
                continue;
            }

            InitLog.println("[DBG:", process->getName(), ":", thread->getName(), "] ", message);
        }

        return CallOk(0zu);
    });
}

static sys::System *gSysSystem = nullptr;

static void initSystem() {
    auto *memory = GetSystemMemory();
    gSysSystem = new sys::System;

    if (OsStatus status = sys::System::create(gVfsRoot, &memory->pageTables(), &memory->pmmAllocator(), gSysSystem)) {
        SysLog.fatalf("Failed to create system: ", OsStatusId(status));
        KM_PANIC("Failed to create system.");
    }
}

sys::System *km::GetSysSystem() {
    return gSysSystem;
}

static km::System GetSystem() {
    return km::System {
        .vfs = gVfsRoot,
        .memory = gMemory,
        .clock = &gClock,
        .sys = gSysSystem,
    };
}

static void AddHandleSystemCalls() {
    AddSystemCall(eOsCallHandleClose, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::HandleClose(&system, context, regs);
    });

    AddSystemCall(eOsCallHandleClone, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::HandleClone(&system, context, regs);
    });

    AddSystemCall(eOsCallHandleWait, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::HandleWait(&system, context, regs);
    });

    AddSystemCall(eOsCallHandleStat, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::HandleStat(&system, context, regs);
    });

    AddSystemCall(eOsCallHandleOpen, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::HandleOpen(&system, context, regs);
    });
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

    AddSystemCall(eOsCallThreadSuspend, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        km::System system = GetSystem();
        return um::ThreadSuspend(&system, context, regs);
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
#if 0
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
#endif
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

    AddSystemCall(eOsCallVmemMap, [](CallContext *context, SystemCallRegisterSet *regs) -> OsCallResult {
        System system = GetSystem();
        auto result = um::VmemMap(&system, context, regs);
        InitLog.dbgf("VmemMap result: ", OsStatusId(result.Status));
        return result;
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

static void enableUmip(bool enable) {
    if (enable) {
        x64::Cr4 cr4 = x64::Cr4::load();
        cr4.set(x64::Cr4::UMIP);
        x64::Cr4::store(cr4);
    }
}

[[noreturn]]
static void enterSchedulerLoop(km::IApic *apic, km::ApicTimer *apicTimer) {
    auto frequency = apicTimer->frequency();
    auto ticks = (frequency * kDefaultTimeSlice.count()) / 1000;
    apic->setTimerDivisor(km::apic::TimerDivide::e1);
    apic->setInitialCount(uint64_t(ticks / si::hertz));

    apic->cfgIvtTimer(km::apic::IvtConfig {
        .vector = km::isr::kTimerVector,
        .polarity = km::apic::Polarity::eActiveHigh,
        .trigger = km::apic::Trigger::eEdge,
        .enabled = true,
        .timer = km::apic::TimerMode::ePeriodic,
    });

    KmIdle();
}

static void startupSmp(const acpi::AcpiTables& rsdt, bool umip, km::ITickSource *tickSource, bool invariantTsc) {
    std::atomic_flag launchScheduler = ATOMIC_FLAG_INIT;

    if constexpr (kEnableSmp) {
        //
        // We provide an atomic flag to the AP cores that we use to signal when the
        // scheduler is ready to be used. The scheduler requires the system to switch
        // to using cpu local isr tables, which must happen after smp startup.
        //
        setupSmp(*GetSystemMemory(), GetCpuLocalApic(), rsdt, [&launchScheduler, umip, tickSource, invariantTsc](IApic *apic) {
            while (!launchScheduler.test()) {
                _mm_pause();
            }

            enableUmip(umip);

            km::ITickSource *bestTickSource = tickSource;

            ApicTimer apicTimer;
            InvariantTsc tsc;

            if (OsStatus status = km::trainApicTimer(apic, bestTickSource, &apicTimer)) {
                InitLog.warnf("Failed to train APIC timer: ", OsStatusId(status));
            } else {
                InitLog.infof("APIC timer frequency: ", apicTimer.refclk());
            }

            if (invariantTsc) {
                if (OsStatus status = km::trainInvariantTsc(bestTickSource, &tsc)) {
                    InitLog.warnf("Failed to train invariant TSC: ", OsStatusId(status));
                } else {
                    InitLog.infof("Invariant TSC frequency: ", tsc.frequency());
                    bestTickSource = &tsc;
                }
            }

            sys::setupApScheduler();
            enterSchedulerLoop(GetCpuLocalApic(), &apicTimer);
        });
    }

    km::EnableCpuLocalIsrTable();

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

static OsStatus launchThread(OsStatus(*entry)(void*), void *arg, stdx::String name) {
    km::StackMappingAllocation stack;
    if (OsStatus status = gMemory->mapStack(kKernelStackSize, PageFlags::eData, &stack)) {
        return status;
    }

    memset(stack.baseAddress(), 0, kKernelStackSize);

    OsThreadCreateInfo createInfo {
        .CpuState = {
            .rdi = (uintptr_t)arg,
            .rbp = 0x0,
            .rsp = std::bit_cast<uintptr_t>(stack.stackBaseAddress() - 0x8),
            .rip = (uintptr_t)entry,
        },
        .Flags = eOsThreadRunning,
    };
    strncpy(createInfo.Name, name.cString(), sizeof(createInfo.Name));
    OsThreadHandle thread = OS_HANDLE_INVALID;
    sys::InvokeContext invoke { gSysSystem, sys::GetCurrentProcess(), sys::GetCurrentThread() };
    if (OsStatus status = sys::SysThreadCreate(&invoke, createInfo, &thread)) {
        return status;
    }

    return OsStatusSuccess;
}

static OsStatus NotificationWork(void *) {
    InitLog.dbgf("Beginning notification work.");
    while (true) {
        gNotificationStream->processAll();
        arch::Intrin::halt();
    }

    return OsStatusSuccess;
}

static OsStatus kernelMasterTask() {
    InitLog.infof("Kernel master task.");

    launchThread(&NotificationWork, gNotificationStream, "NOTIFY");

    OsProcessHandle hInit = OS_HANDLE_INVALID;
    sys::InvokeContext invoke { gSysSystem, sys::GetCurrentProcess(), sys::GetCurrentThread() };
    if (OsStatus status = launchInitProcess(&invoke, &hInit)) {
        InitLog.fatalf("Failed to create INIT process: ", OsStatusId(status));
        KM_PANIC("Failed to create init process.");
    }

    while (true) {
        // OsInstant now;
        // gClock.time(&now);
        // scheduler->tick(now);
    }

    return OsStatusSuccess;
}

static void configurePs2Controller(const acpi::AcpiTables& rsdt, IoApicSet& ioApicSet, const IApic *apic, LocalIsrTable *ist) {
    static hid::Ps2Controller ps2Controller;

    bool has8042 = rsdt.has8042Controller();

    hid::InitPs2HidStream(gNotificationStream);

    if (has8042) {
        hid::Ps2ControllerResult result = hid::EnablePs2Controller();
        InitLog.infof("PS/2 controller: ", result.status);
        if (result.status == hid::Ps2ControllerStatus::eOk) {
            ps2Controller = result.controller;

            InitLog.infof("PS/2 keyboard: ", present(ps2Controller.hasKeyboard()));
            InitLog.infof("PS/2 mouse: ", present(ps2Controller.hasMouse()));
        } else {
            return;
        }
    } else {
        InitLog.infof("No PS/2 controller found.");
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

    vfs::VfsPath hidPs2DevicePath{OS_DEVICE_PS2_KEYBOARD};

    {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(hidPs2DevicePath.parent(), &node)) {
            VfsLog.fatalf("Failed to create ", hidPs2DevicePath.parent(), " folder: ", OsStatusId(status));
            KM_PANIC("Failed to create keyboar device folder.");
        }
    }

    {
        auto device = sm::rcuMakeShared<dev::HidKeyboardDevice>(gVfsRoot->domain());
        if (OsStatus status = gVfsRoot->mkdevice(hidPs2DevicePath, device)) {
            VfsLog.fatalf("Failed to create ", hidPs2DevicePath, " device: ", OsStatusId(status));
            KM_PANIC("Failed to create keyboard device.");
        }

        gNotificationStream->subscribe(hid::GetHidPs2Topic(), device.get());
    }
}

static void createDisplayDevice() {
    vfs::VfsPath ddiPath{OS_DEVICE_DDI_RAMFB};

    {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        if (OsStatus status = gVfsRoot->mkpath(ddiPath.parent(), &node)) {
            VfsLog.fatalf("Failed to create ", ddiPath.parent(), " folder: ", OsStatusId(status));
            KM_PANIC("Failed to create display device folder.");
        }
    }

    {
        auto device = sm::rcuMakeShared<dev::DisplayDevice>(gVfsRoot->domain(), gVgaAppender.getCanvas());
        if (OsStatus status = gVfsRoot->mkdevice(ddiPath, device)) {
            VfsLog.fatalf("Failed to create ", ddiPath, " device: ", OsStatusId(status));
            KM_PANIC("Failed to create display device.");
        }
    }
}

static void launchKernelProcess(km::ITickSource *tickSource) {
    sys::ProcessCreateInfo createInfo {
        .name = stdx::StringView("SYSTEM\0"),
        .state = eOsProcessSupervisor,
    };
    std::unique_ptr<sys::ProcessHandle> system;
    OsStatus status = sys::SysCreateRootProcess(gSysSystem, createInfo, std::out_ptr(system));
    if (status != OsStatusSuccess) {
        InitLog.fatalf("Failed to create SYSTEM process: ", OsStatusId(status));
        KM_PANIC("Failed to create SYSTEM process.");
    }

    InitLog.infof("Creating master task.");

    km::StackMappingAllocation stack;
    if (OsStatus status = gMemory->mapStack(kKernelStackSize, PageFlags::eData, &stack)) {
        InitLog.fatalf("Failed to map kernel stack: ", OsStatusId(status));
        KM_PANIC("Failed to map kernel stack.");
    }

    memset(stack.baseAddress(), 0, kKernelStackSize);

    OsThreadCreateInfo threadInfo {
        .Name = "SYSTEM MASTER TASK",
        .CpuState = {
            .rsi = reinterpret_cast<uintptr_t>(tickSource),
            .rdi = static_cast<uintptr_t>(km::GetCurrentCoreId()),
            .rbp = 0x0,
            .rsp = std::bit_cast<uintptr_t>(stack.stackBaseAddress() - 0x8),
            .rip = (uintptr_t)&kernelMasterTask,
        },
        .Flags = eOsThreadRunning,
    };

    OsThreadHandle thread = OS_HANDLE_INVALID;
    sys::InvokeContext invoke { gSysSystem, system->getProcess() };
    status = sys::SysThreadCreate(&invoke, threadInfo, &thread);
    if (status != OsStatusSuccess) {
        InitLog.fatalf("Failed to create SYSTEM thread: ", OsStatusId(status));
        KM_PANIC("Failed to create SYSTEM thread.");
    }

    InitLog.fatalf("Create master thread.");
}

static void initVfs() {
    MountRootVfs();
    MountVolatileFolder();
}

static void createVfsDevices(const km::SmBiosTables *smbios, const acpi::AcpiTables *acpi, MemoryRangeEx initrd) {
    CreatePlatformVfsNodes(smbios, acpi);
    MountInitArchive(initrd, GetSystemMemory()->pageTables());
}

static void initUserApi() {
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

static void displayHpetInfo(const km::HighPrecisionTimer& hpet) {
    InitLog.println("|---- HPET -----------------");
    InitLog.println("| Property      | Value");
    InitLog.println("|---------------+-----------");
    InitLog.println("| Vendor        | ", hpet.vendor());
    InitLog.println("| Timer count   | ", hpet.timerCount());
    InitLog.println("| Clock         | ", hpet.refclk());
    InitLog.println("| Counter size  | ", hpet.counterSize() == hpet::Width::DWORD ? "32"_sv : "64"_sv);
    InitLog.println("| Revision      | ", hpet.revision());
    InitLog.println("| Ticks         | ", hpet.ticks());

    auto comparators = hpet.comparators();
    for (size_t i = 0; i < comparators.size(); i++) {
        const auto& comparator = comparators[i];
        auto config = comparator.config();
        InitLog.println("| Comparator    | ", i);
        InitLog.println("|  - Route Mask | ", comparator.routeMask());
        InitLog.println("|  - Counter    | ", comparator.counter());
        InitLog.println("|  - FSB        | ", comparator.fsbIntDelivery());
        InitLog.println("|  - Periodic   | ", comparator.periodicSupport());
        InitLog.println("|  - Width      | ", comparator.width() == hpet::Width::DWORD ? "32"_sv : "64"_sv);
        InitLog.println("|  - Enabled    | ", config.enable);
        InitLog.println("|  - Period     | ", config.period);
        InitLog.println("|  - Mode       | ", config.mode == hpet::Width::DWORD ? "32"_sv : "64"_sv);
        InitLog.println("|  - Trigger    | ", config.trigger == apic::Trigger::eLevel ? "Level"_sv : "Edge"_sv);
        InitLog.println("|  - Route      | ", config.ioApicRoute);
    }
}

void LaunchKernel(boot::LaunchInfo launch) {
    normalizeProcessorState();
    initBootTerminal(launch.framebuffers);
    auto [hvInfo, hasDebugPort] = queryHostHypervisor();

#if 0
    TestCanvas(gDirectTerminalLog.get().display());
    KmHalt();
#endif

    ProcessorInfo processor = GetProcessorInfo();
    enableUmip(processor.umip());

    initPortDelay(hvInfo);

    ComPortInfo com2Info = {
        .port = km::com::kComPort2,
        .divisor = km::com::kBaud9600,
    };

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
    };

    SerialPortStatus com1Status = initSerialPort(com1Info);

    if (OsStatus status = debug::InitDebugStream(com2Info)) {
        InitLog.warnf("Failed to initialize debug stream: ", OsStatusId(status));
    }

    displaySystemInfo(launch, hvInfo, processor, hasDebugPort, com1Status, com1Info);

    gBootGdt = getBootGdt();
    setupInitialGdt();
    InitLog.info("GDT initialized");

    XSaveConfig xsaveConfig {
        .target = kEnableXSave ? SaveMode::eXSave : SaveMode::eFxSave,
        .features = XSaveMask(x64::FPU, x64::SSE, x64::AVX) | x64::kSaveAvx512,
        .cpuInfo = &processor,
    };

    initFpuSave(xsaveConfig);

    //
    // Once we have the initial gdt setup we create the global allocator.
    // The IDT depends on the allocator to create its global ISR table.
    //
    Stage2MemoryInfo *stage2 = InitStage2Memory(launch, processor);
    gMemory = stage2->memory;

    LogQueue::initGlobalLogQueue(128);

    initStage1Idt(SystemGdt::eLongModeCode);
    enableInterrupts();

    initVfs();

    SmBiosLoadOptions smbiosOptions {
        .smbios32Address = launch.smbios32Address,
        .smbios64Address = launch.smbios64Address,
    };

    km::SmBiosTables smbios{};

    if (OsStatus status = findSmbiosTables(smbiosOptions, gMemory->pageTables(), &smbios)) {
        BiosLog.warnf("Failed to find SMBIOS tables: ", OsStatusId(status));
    }

    //
    // On Oracle VirtualBox the COM1 port is functional but fails the loopback test.
    // If we are running on VirtualBox, retry the serial port initialization without the loopback test.
    //
    if (com1Status == SerialPortStatus::eLoopbackTestFailed && smbios.isOracleVirtualBox()) {
        updateSerialPort(com1Info);
    }

    arch::Intrin::LongJumpState jmp{};
    if (auto value = arch::Intrin::setjmp(&jmp)) {
        TestLog.dbgf("Long jump: ", value);
    } else {
        arch::Intrin::longjmp(&jmp, 1);
    }

    bool useX2Apic = kUseX2Apic && processor.x2apic();

    auto lapic = enableBootApic(gMemory->pageTables(), useX2Apic);

    acpi::AcpiTables rsdt = acpi::setupAcpi(launch.rsdpAddress, gMemory->pageTables());
    const acpi::Fadt *fadt = rsdt.fadt();
    initCmos(fadt->century);

    uint32_t ioApicCount = rsdt.ioApicCount();
    KM_CHECK(ioApicCount > 0, "No IOAPICs found.");
    IoApicSet ioApicSet{ rsdt.madt(), gMemory->pageTables() };

    std::unique_ptr<pci::IConfigSpace> config;
    if (OsStatus status = pci::setupConfigSpace(rsdt.mcfg(), gMemory->pageTables(), std::out_ptr(config))) {
        InitLog.warnf("Failed to initialize PCI config space: ", OsStatusId(status));
    }

    pci::ProbeConfigSpace(config.get(), rsdt.mcfg());

    km::LocalIsrTable *ist = GetLocalIsrTable();
    sys::installSchedulerIsr();

    initSystem();

    const IsrEntry *timerInt = ist->allocate([](km::IsrContext *ctx) noexcept [[clang::reentrant]] -> km::IsrContext {
        km::IApic *apic = km::GetCpuLocalApic();
        apic->eoi();
        return *ctx;
    });
    uint8_t timerIdx = ist->index(timerInt);
    InitLog.dbgf("Timer ISR: ", timerIdx);

    stdx::StaticVector<km::ITickSource*, 4> tickSources;

    km::IntervalTimer pit = km::setupPit(100 * si::hertz, ioApicSet, lapic.pointer(), timerIdx);

    tickSources.add(&pit);
    km::ITickSource *tickSource = &pit;

    km::HighPrecisionTimer hpet;
    if (OsStatus status = km::setupHpet(rsdt, gMemory->pageTables(), &hpet)) {
        InitLog.warnf("Failed to initialize HPET: ", OsStatusId(status));
    } else {
        hpet.enable(true);
        tickSources.add(&hpet);
        tickSource = &hpet;

        displayHpetInfo(hpet);
    }

    InitLog.dbgf("Training APIC timer.");
    km::ITickSource *clockTicker = tickSource;
    ApicTimer apicTimer;
    InvariantTsc tsc;

    if (OsStatus status = km::trainApicTimer(lapic.pointer(), tickSources.back(), &apicTimer)) {
        InitLog.warnf("Failed to train APIC timer: ", OsStatusId(status));
    } else {
        InitLog.infof("APIC timer frequency: ", apicTimer.refclk());
        tickSources.add(&apicTimer);
    }

    if (processor.invariantTsc) {
        if (OsStatus status = km::trainInvariantTsc(tickSource, &tsc)) {
            InitLog.warnf("Failed to train invariant TSC: ", OsStatusId(status));
        } else {
            InitLog.infof("Invariant TSC frequency: ", tsc.frequency());
            tickSources.add(&tsc);
            clockTicker = &tsc;
        }
    }

    ioApicSet.clearLegacyRedirect(irq::kTimer);

    sys::setupGlobalScheduler(kEnableSmp, rsdt, &gSysSystem->mScheduler);

    startupSmp(rsdt, processor.umip(), clockTicker, processor.invariantTsc);

    DateTime time = readCmosClock();
    ClockLog.infof("Current time: ", time);

    gClock = Clock { time, clockTicker };

    createVfsDevices(&smbios, &rsdt, launch.initrd);
    initUserApi();
    createNotificationQueue();
    configurePs2Controller(rsdt, ioApicSet, lapic.pointer(), ist);
    createDisplayDevice();

    launchKernelProcess(clockTicker);

    enterSchedulerLoop(GetCpuLocalApic(), &apicTimer);

    KM_PANIC("Test bugcheck.");
}
