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

#include "kernel.h"
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
#include "memory/virtual_allocator.hpp"
#include "panic.hpp"
#include "pat.hpp"
#include "processor.hpp"
#include "smp.hpp"
#include "syscall.hpp"
#include "thread.hpp"
#include "uart.hpp"
#include "smbios.hpp"
#include "pci.hpp"

#include "memory/layout.hpp"
#include "memory/allocator.hpp"

#include "allocator/tlsf.hpp"

#include "arch/intrin.hpp"

#include "kernel.hpp"

#include "util/memory.hpp"

using namespace km;
using namespace stdx::literals;

static constexpr bool kUseX2Apic = true;
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

size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void debug_prints(const char *message)
{
    size_t len = strlen(message);
    KmDebugMessage(stdx::StringView(message, message + len));
}

void debug_printull(uint64_t value)
{
    KmDebugMessage(km::Hex(value).pad(16));
}

// load bearing constinit, clang has a bug in c++26 mode
// where it doesnt emit a warning for global constructors in all cases.
constinit static SerialLog gSerialLog;
constinit static TerminalLog<DirectTerminal> gDirectTerminalLog;
constinit static TerminalLog<BufferedTerminal> gBufferedTerminalLog;
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

constinit km::ThreadLocal<SystemGdt> km::tlsSystemGdt;
constinit km::ThreadLocal<x64::TaskStateSegment> km::tlsTaskState;

static SystemGdt gBootGdt;

void SetupInitialGdt(void) {
    KmInitGdt(gBootGdt.entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
}

void SetupApGdt(void) {
    // Save the gs/fs/kgsbase values, as setting the GDT will clear them
    TlsRegisters tls = LoadTlsRegisters();

    tlsTaskState = x64::TaskStateSegment {
        .iopbOffset = sizeof(x64::TaskStateSegment),
    };
    tlsSystemGdt = KmGetSystemGdt(&tlsTaskState);
    KmInitGdt(tlsSystemGdt->entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
    __ltr(SystemGdt::eTaskState0 * 0x8);

    StoreTlsRegisters(tls);
}

static PageMemoryTypeLayout KmSetupPat(void) {
    if (!x64::HasPatSupport()) {
        return PageMemoryTypeLayout { };
    }

    x64::PageAttributeTable pat = x64::PageAttributeTable::get();

    for (uint8_t i = 0; i < pat.count(); i++) {
        km::MemoryType type = pat.getEntry(i);
        KmDebugMessage("[INIT] PAT[", i, "]: ", type, "\n");
    }

    enum {
        kEntryWriteBack,
        kEntryWriteThrough,
        kEntryUncachedOverridable,
        kEntryUncached,
        kEntryWriteCombined,
        kEntryWriteProtect,
    };

    pat.setEntry(kEntryUncached, MemoryType::eUncached);
    pat.setEntry(kEntryWriteCombined, MemoryType::eWriteCombine);
    pat.setEntry(kEntryWriteThrough, MemoryType::eWriteThrough);
    pat.setEntry(kEntryWriteBack, MemoryType::eWriteBack);
    pat.setEntry(kEntryWriteProtect, MemoryType::eWriteProtect);
    pat.setEntry(kEntryUncachedOverridable, MemoryType::eUncachedOverridable);

    // PAT[6] and PAT[7] are unused for now, so just set them to UC-
    pat.setEntry(6, MemoryType::eUncachedOverridable);
    pat.setEntry(7, MemoryType::eUncachedOverridable);

    return PageMemoryTypeLayout {
        .deferred = kEntryUncachedOverridable,
        .uncached = kEntryUncached,
        .writeCombined = kEntryWriteCombined,
        .writeThrough = kEntryWriteThrough,
        .writeProtect = kEntryWriteProtect,
        .writeBack = kEntryWriteBack,
    };
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

static void KmWriteMtrrs(const km::PageBuilder& pm) {
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

static void WriteMemoryMap(std::span<const boot::MemoryRegion> memmap) {
    KmDebugMessage("[INIT] ", memmap.size(), " memory map entries.\n");

    KmDebugMessage("| Entry | Address            | Size               | Type\n");
    KmDebugMessage("|-------+--------------------+--------------------+-----------------------\n");

    for (size_t i = 0; i < memmap.size(); i++) {
        boot::MemoryRegion entry = memmap[i];
        MemoryRange range = entry.range;

        KmDebugMessage("| ", Int(i).pad(4, '0'), "  | ", Hex(range.front.address).pad(16, '0'), " | ", Hex(range.size()).pad(16, '0'), " | ", entry.type, "\n");
    }
}

static void WriteSystemMemory(std::span<const boot::MemoryRegion> memmap, const SystemMemory& memory) {
    WriteMemoryMap(memmap);

    uint64_t usableMemory = 0;
    for (const MemoryRange& range : memory.layout.available) {
        usableMemory += range.size();
    }

    uint64_t reclaimableMemory = 0;
    for (const MemoryRange& range : memory.layout.reclaimable) {
        reclaimableMemory += range.size();
    }

    KmDebugMessage("[INIT] Usable memory: ", sm::bytes(usableMemory), ", Reclaimable memory: ", sm::bytes(reclaimableMemory), "\n");
}

struct KernelLayout {
    /// @brief Virtual address space reserved for the kernel.
    /// Space for dynamic allocations, kernel heap, kernel stacks, etc.
    VirtualRange data;

    /// @brief All framebuffers that are available.
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
};

extern "C" {
    extern char __kernel_start[];
    extern char __kernel_end[];
}

static size_t GetKernelSize() {
    return (uintptr_t)__kernel_end - (uintptr_t)__kernel_start;
}

static KernelLayout BuildKernelLayout(uintptr_t vaddrbits, km::PhysicalAddress kernelPhysicalBase, const void *kernelVirtualBase, std::span<const boot::FrameBuffer> displays) {
    size_t framebuffersSize = std::accumulate(displays.begin(), displays.end(), 0, [](size_t sum, const boot::FrameBuffer& framebuffer) {
        return sum + framebuffer.size();
    });

    // reserve the top 1/4 of the address space for the kernel
    uintptr_t dataBack = -(1ull << (vaddrbits - 1));
    uintptr_t dataFront = -(1ull << (vaddrbits - 2));

    VirtualRange data = { (void*)dataFront, (void*)dataBack };

    uintptr_t offset = dataFront;

    auto reserve = [&](size_t size) {
        uintptr_t back = offset;
        offset += sm::roundup(size, x64::kPageSize);

        return VirtualRange { (void*)back, (void*)offset };
    };

    VirtualRange framebuffers = reserve(framebuffersSize);

    AddressMapping kernel = { kernelVirtualBase, kernelPhysicalBase, GetKernelSize() };

    KmDebugMessage("[INIT] Kernel layout:\n");
    KmDebugMessage("[INIT] Data         : ", data, "\n");
    KmDebugMessage("[INIT] Framebuffers : ", framebuffers, "\n");
    KmDebugMessage("[INIT] Kernel       : ", kernel, "\n");

    KernelLayout layout = {
        .data = data,
        .framebuffers = framebuffers,
        .kernel = kernel,
    };

    return layout;
}

static constexpr size_t kEarlyAllocSize = sm::megabytes(1).bytes();
static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

struct EarlyMemory {
    mem::TlsfAllocator allocator;
    std::span<boot::MemoryRegion> memmap;
};

static boot::MemoryRegion FindEarlyAllocatorRegion(std::span<const boot::MemoryRegion> memmap, size_t size) {
    for (const boot::MemoryRegion& entry : memmap) {
        if (entry.type != boot::MemoryRegion::eUsable)
            continue;

        if (entry.range.front < kLowMemory)
            continue;

        if (entry.range.size() < size)
            continue;

        return entry;
    }

    KM_PANIC("No suitable memory region found for early allocator.");
}

static EarlyMemory *AllocateEarlyMemory(void *vaddr, size_t size) {
    mem::TlsfAllocator allocator { vaddr, size };

    EarlyMemory *mem = allocator.box(EarlyMemory { });
    mem->allocator = std::move(allocator);

    return mem;
}

static std::tuple<EarlyMemory*, km::AddressMapping> CreateEarlyAllocator(std::span<const boot::MemoryRegion> memmap, uintptr_t hhdmOffset) {
    boot::MemoryRegion region = FindEarlyAllocatorRegion(memmap, kEarlyAllocSize);
    km::MemoryRange range = { region.range.front, region.range.front + kEarlyAllocSize };

    void *vaddr = std::bit_cast<void*>(region.range.front + hhdmOffset);
    EarlyMemory *memory = AllocateEarlyMemory(vaddr, kEarlyAllocSize);

    boot::MemoryRegion *memoryMap = memory->allocator.allocateArray<boot::MemoryRegion>(memmap.size() + 1);
    std::copy(memmap.begin(), memmap.end(), memoryMap);
    memoryMap[memmap.size()] = boot::MemoryRegion { boot::MemoryRegion::eKernelRuntimeData, range };

    for (size_t i = 0; i < memmap.size(); i++) {
        boot::MemoryRegion& entry = memoryMap[i];
        if (entry.range.overlaps(range) || entry.range.intersects(range)) {
            entry.range = entry.range.cut(range);
        }
    }

    memory->memmap = std::span(memoryMap, memmap.size() + 1);

    std::sort(memory->memmap.begin(), memory->memmap.end(), [](const boot::MemoryRegion& a, const boot::MemoryRegion& b) {
        return a.range.front < b.range.front;
    });

    return std::make_tuple(memory, km::AddressMapping { vaddr, range.front, kEarlyAllocSize });
}

static SystemMemory *SetupKernelMemory(
    uintptr_t maxpaddr,
    const KernelLayout& layout,
    km::AddressMapping stack,
    uintptr_t hhdmOffset,
    std::span<const boot::FrameBuffer> framebuffers,
    std::span<const boot::MemoryRegion> memmap
) {
    PageMemoryTypeLayout pat = KmSetupPat();

    auto [earlyMemory, earlyRegion] = CreateEarlyAllocator(memmap, hhdmOffset);

    WriteMemoryMap(earlyMemory->memmap);

    PageBuilder pm = PageBuilder { maxpaddr, hhdmOffset, pat };
    KmWriteMtrrs(pm);

    mem::IAllocator *alloc = &earlyMemory->allocator;

    SystemMemory *memory = alloc->construct<SystemMemory>(SystemMemoryLayout::from(earlyMemory->memmap, &earlyMemory->allocator), pm, &earlyMemory->allocator);

    memory->pmm.markUsed(earlyRegion.physicalRange());
    memory->pmm.markUsed(stack.physicalRange());

    WriteSystemMemory(earlyMemory->memmap, *memory);

    // initialize our own page tables and remap everything into it
    auto [kernelVirtualBase, kernelPhysicalBase, _] = layout.kernel;
    KmMapKernel(memory->vmm, kernelPhysicalBase, kernelVirtualBase);

    // move our stack out of reclaimable memory
    KmMapMemory(memory->vmm, stack.paddr - stack.size, (void*)((uintptr_t)stack.vaddr - stack.size), stack.size, PageFlags::eData, MemoryType::eWriteBack);

    // map the early allocator region
    KmMapMemory(memory->vmm, earlyRegion.paddr, earlyRegion.vaddr, earlyRegion.size, PageFlags::eData, MemoryType::eWriteBack);

    // remap framebuffers

    uintptr_t framebufferBase = (uintptr_t)layout.framebuffers.front;
    for (const boot::FrameBuffer& framebuffer : framebuffers) {
        // remap the framebuffer into its final location
        KmMapMemory(memory->vmm, framebuffer.paddr, (void*)framebufferBase, framebuffer.size(), PageFlags::eData, MemoryType::eWriteCombine);

        framebufferBase += framebuffer.size();
    }

    // once it is safe to remap the boot memory, do so
    KmReclaimBootMemory(memory->pager, memory->vmm);

    // can't log anything here as we need to move the framebuffer first

    // Now update the terminal to use the new memory layout
    if (!framebuffers.empty()) {
        gDirectTerminalLog.get()
            .display()
            .setAddress(layout.getFrameBuffer(framebuffers, 0));
    }

    memory->layout.reclaimBootMemory();

    return memory;
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

static km::IntController KmEnableLocalApic(km::SystemMemory& memory, km::IsrAllocator& isrs, bool useX2Apic) {
    km::IntController pic = KmInitBspApic(memory, useX2Apic);

    // setup tls now that we have the lapic id

    km::InitTlsRegion(memory);

    SetDebugLogLock(DebugLogLockType::eSpinLock);
    km::InitKernelThread(pic);

    uint8_t spuriousVec = isrs.allocateIsr();
    KmDebugMessage("[INIT] APIC ID: ", pic->id(), ", Version: ", pic->version(), ", Spurious vector: ", spuriousVec, "\n");

    KmInstallIsrHandler(spuriousVec, [](km::IsrContext *ctx) -> void* {
        KmDebugMessage("[ISR] Spurious interrupt: ", ctx->vector, "\n");
        km::IIntController *pic = km::GetCurrentCoreIntController();
        pic->eoi();
        return ctx;
    });

    pic->setSpuriousVector(spuriousVec);

    pic->enable();

    if (kSelfTestApic) {
        uint8_t isr = isrs.allocateIsr();

        KmIsrHandler old = KmInstallIsrHandler(isr, [](km::IsrContext *context) -> void* {
            KmDebugMessage("[SELFTEST] Handled isr: ", context->vector, "\n");
            km::IIntController *pic = km::GetCurrentCoreIntController();
            pic->eoi();
            return context;
        });

        pic->sendIpi(apic::IcrDeliver::eSelf, isr);
        pic->sendIpi(apic::IcrDeliver::eSelf, isr);
        pic->sendIpi(apic::IcrDeliver::eSelf, isr);

        KmInstallIsrHandler(isr, old);
        isrs.releaseIsr(isr);
    }

    return pic;
}

static void KmInitBootTerminal(std::span<const boot::FrameBuffer> framebuffers) {
    if (framebuffers.empty()) return;

    boot::FrameBuffer framebuffer = framebuffers.front();
    km::Canvas display { framebuffer, (uint8_t*)(framebuffer.vaddr) };
    gDirectTerminalLog = DirectTerminal(display);
    gLogTargets.add(&gDirectTerminalLog);
}

static void KmInitBootBufferedTerminal(std::span<const boot::FrameBuffer> framebuffers, SystemMemory& memory) {
    if (framebuffers.empty()) return;

    KmDebugMessage("[INIT] Deleting unbuffered terminal\n");

    gLogTargets.erase(&gDirectTerminalLog);

    KmDebugMessage("[INIT] Initializing buffered terminal\n");

    gBufferedTerminalLog = BufferedTerminal(gDirectTerminalLog.get(), memory);
    gLogTargets.add(&gBufferedTerminalLog);

    KmDebugMessage("[INIT] Buffered terminal initialized\n");
}

static void KmUpdateSerialPort(ComPortInfo info) {
    info.skipLoopbackTest = true;
    if (OpenSerialResult com1 = OpenSerial(info); com1.status == SerialPortStatus::eOk) {
        gSerialLog = SerialLog(com1.port);
        gLogTargets.add(&gSerialLog);
    }
}

static SerialPortStatus KmInitSerialPort(ComPortInfo info) {
    if (OpenSerialResult com1 = OpenSerial(info)) {
        return com1.status;
    } else {
        gSerialLog = SerialLog(com1.port);
        gLogTargets.add(&gSerialLog);
        return SerialPortStatus::eOk;
    }
}

static void DumpIsrContext(const km::IsrContext *context, stdx::StringView message) {
    if (km::IsTlsSetup()) {
        KmDebugMessage("\n[BUG] ", message, " - On ", km::GetCurrentCoreId(), "\n");
    } else {
        KmDebugMessage("\n[BUG] ", message, "\n");
    }

    DumpIsrState(context);
}

static void InstallExceptionHandlers(void) {
    KmInstallIsrHandler(0x0, [](km::IsrContext *context) -> void* {
        DumpIsrContext(context, "Divide by zero (#DE)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    KmInstallIsrHandler(0x2, [](km::IsrContext *context) -> void* {
        KmDebugMessage("[INT] Non-maskable interrupt (#NM)\n");
        DumpIsrState(context);
        return context;
    });

    KmInstallIsrHandler(0x6, [](km::IsrContext *context) -> void* {
        DumpIsrContext(context, "Invalid opcode (#UD)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    KmInstallIsrHandler(0x8, [](km::IsrContext *context) -> void* {
        DumpIsrContext(context, "Double fault (#DF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    KmInstallIsrHandler(0xD, [](km::IsrContext *context) -> void* {
        DumpIsrContext(context, "General protection fault (#GP)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    KmInstallIsrHandler(0xE, [](km::IsrContext *context) -> void* {
        KmDebugMessage("[BUG] CR2: ", Hex(__get_cr2()).pad(16, '0'), "\n");
        DumpIsrContext(context, "Page fault (#PF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });
}

static void InitPortDelay(const std::optional<HypervisorInfo>& hvInfo) {
    x64::PortDelay delay = hvInfo.transform([](const HypervisorInfo& hv) { return hv.isKvm(); }).value_or(false)
        ? x64::PortDelay::eNone
        : x64::PortDelay::ePostCode;

    KmSetPortDelayMethod(delay);
}

extern const char _binary_init_elf_start[];
extern const char _binary_init_elf_end[];

static std::span<const uint8_t> GetInitProgram() {
    return { (const uint8_t*)_binary_init_elf_start, (const uint8_t*)_binary_init_elf_end };
}

static void LogSystemInfo(
    uintptr_t hhdmOffset,
    std::span<const boot::FrameBuffer> framebuffers,
    std::optional<HypervisorInfo> hvInfo,
    ProcessorInfo processor,
    bool hasDebugPort,
    SerialPortStatus com1Status,
    const ComPortInfo& com1Info) {
    KmDebugMessage("[INIT] CR0: ", x64::Cr0::load(), "\n");
    KmDebugMessage("[INIT] CR4: ", x64::Cr4::load(), "\n");
    KmDebugMessage("[INIT] HHDM: ", Hex(hhdmOffset).pad(16, '0'), "\n");

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

    for (size_t i = 0; i < framebuffers.size(); i++) {
        const boot::FrameBuffer& display = framebuffers[i];
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
}

void KmLaunchEx(boot::LaunchInfo launch) {
    x64::Cr0 cr0 = x64::Cr0::load();
    cr0.set(x64::Cr0::WP | x64::Cr0::NE);
    x64::Cr0::store(cr0);

    kGsBase.store(0);

    SetDebugLogLock(DebugLogLockType::eNone);

    KmInitBootTerminal(launch.framebuffers);

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

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
        .skipLoopbackTest = false,
    };

    SerialPortStatus com1Status = KmInitSerialPort(com1Info);

    LogSystemInfo(launch.hhdmOffset, launch.framebuffers, hvInfo, processor, hasDebugPort, com1Status, com1Info);

    gBootGdt = KmGetBootGdt();
    SetupInitialGdt();

    km::IsrAllocator isrs;
    KmInitInterrupts(isrs, SystemGdt::eLongModeCode);
    InstallExceptionHandlers();
    EnableInterrupts();

    KernelLayout layout = BuildKernelLayout(
        processor.maxvaddr,
        launch.kernelPhysicalBase,
        launch.kernelVirtualBase,
        launch.framebuffers
    );
    SystemMemory *memory = SetupKernelMemory(
        processor.maxpaddr, layout, launch.stack,
        launch.hhdmOffset,
        launch.framebuffers, launch.memmap
    );

    PlatformInfo platform = KmGetPlatformInfo(launch.smbios32Address, launch.smbios64Address, *memory);

    // On Oracle VirtualBox the COM1 port is functional but fails the loopback test.
    // If we are running on VirtualBox, retry the serial port initialization without the loopback test.
    if (com1Status == SerialPortStatus::eLoopbackTestFailed && platform.isOracleVirtualBox()) {
        KmUpdateSerialPort(com1Info);
    }

    // KmInitBootBufferedTerminal(launch.framebuffers, memory);

    bool useX2Apic = kUseX2Apic && processor.has2xApic;

    km::IntController lapic = KmEnableLocalApic(*memory, isrs, useX2Apic);

    acpi::AcpiTables rsdt = InitAcpi(launch.rsdpAddress, *memory);
    const acpi::Fadt *fadt = rsdt.fadt();
    InitCmos(fadt->century);

    uint32_t ioApicCount = rsdt.ioApicCount();
    KM_CHECK(ioApicCount > 0, "No IOAPICs found.");

    for (uint32_t i = 0; i < ioApicCount; i++) {
        IoApic ioapic = rsdt.mapIoApic(*memory, 0);

        KmDebugMessage("[INIT] IOAPIC ", i, " ID: ", ioapic.id(), ", Version: ", ioapic.version(), "\n");
        KmDebugMessage("[INIT] ISR base: ", ioapic.isrBase(), ", Inputs: ", ioapic.inputCount(), "\n");
    }

    bool has8042 = rsdt.has8042Controller();

    hid::Ps2Controller ps2Controller;

    pci::ProbeConfigSpace();

    KmInitSmp(*memory, lapic.pointer(), rsdt, useX2Apic);
    SetDebugLogLock(DebugLogLockType::eRecursiveSpinLock);

    // Setup gdt that contains a TSS for this core
    SetupApGdt();

    km::SetupUserMode(*memory);

    std::span init = GetInitProgram();

    void *initMemory = memory->allocate(0x8000);
    mem::TlsfAllocator allocator(initMemory, 0x8000);
    auto process = LoadElf(init, "init.elf", 1, *memory, &allocator);
    KM_CHECK(process.has_value(), "Failed to load init.elf");

    DateTime time = ReadRtc();
    KmDebugMessage("[INIT] Current time: ", time.year, "-", time.month, "-", time.day, "T", time.hour, ":", time.minute, ":", time.second, "Z\n");

    km::EnterUserMode(process->main.state);

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
