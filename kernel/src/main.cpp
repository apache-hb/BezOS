// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "apic.hpp"

#include "arch/cr0.hpp"
#include "arch/cr4.hpp"

#include "delay.hpp"
#include "display.hpp"
#include "gdt.hpp"
#include "hypervisor.hpp"
#include "isr.hpp"
#include "memory.hpp"
#include "pat.hpp"
#include "smp.hpp"
#include "std/spinlock.hpp"
#include "uart.hpp"
#include "acpi.hpp"
#include "smbios.hpp"

#include "memory/layout.hpp"
#include "memory/allocator.hpp"

#include "arch/intrin.hpp"

#include "kernel.hpp"

#include "util/logger.hpp"
#include "util/memory.hpp"

using namespace km;
using namespace stdx::literals;

class SerialLog final : public ILogTarget {
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

class DebugPortLog final : public ILogTarget {
    void write(stdx::StringView message) override {
        for (char c : message) {
            __outbyte(0xE9, c);
        }
    }
};

class TerminalLog final : public ILogTarget {
    BufferedTerminal mTerminal;

public:
    constexpr TerminalLog()
        : mTerminal(sm::noinit{})
    { }

    constexpr TerminalLog(Canvas display, SystemMemory& memory)
        : mTerminal(display, memory)
    { }

    void write(stdx::StringView message) override {
        mTerminal.print(message);
    }
};

class EarlyTerminalLog final : public ILogTarget {
    DisplayTerminal mTerminal;

public:
    constexpr EarlyTerminalLog()
        : mTerminal(sm::noinit{})
    { }

    constexpr EarlyTerminalLog(Canvas display)
        : mTerminal(display)
    { }

    void write(stdx::StringView message) override {
        mTerminal.print(message);
    }
};

// load bearing constinit, clang has a bug in c++26 mode
// where it doesnt emit a warning for global constructors in all cases.
constinit static SerialLog gSerialLog;
constinit static TerminalLog gTerminalLog;
constinit static DebugPortLog gDebugPortLog;
constinit static EarlyTerminalLog gEarlyTerminalLog;

constinit static stdx::StaticVector<ILogTarget*, 4> gLogTargets;

constinit static LocalApic gLocalApic;

// qemu e9 port check - i think bochs does something else
static bool KmTestDebugPort(void) {
    return __inbyte(0xE9) == 0xE9;
}

constinit static stdx::SpinLock gLogLock;

void KmBeginWrite() {
    gLogLock.lock();
}

void KmEndWrite() {
    gLogLock.unlock();
}

void KmDebugWrite(stdx::StringView value) {
    for (ILogTarget *target : gLogTargets) {
        target->write(value);
    }
}

static SystemGdt gSystemGdt;

static void KmSetupBspGdt(void) {
    gSystemGdt = KmGetSystemGdt();
    KmInitGdt(gSystemGdt.entries, SystemGdt::eCount, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
}

void KmSetupApGdt(void) {
    KmInitGdt(gSystemGdt.entries, SystemGdt::eCount, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
}

static PageMemoryTypeLayout KmSetupPat(void) {
    if (!x64::HasPatSupport()) {
        return PageMemoryTypeLayout { };
    }

    x64::PageAttributeTable pat;

    constexpr uint8_t kEntryUncached = 0;
    constexpr uint8_t kEntryWriteCombined = 1;
    constexpr uint8_t kEntryWriteThrough = 2;
    constexpr uint8_t kEntryWriteBack = 3;
    constexpr uint8_t kEntryWriteProtect = 4;
    constexpr uint8_t kEntryUncachedOverridable = 5;

    pat.setEntry(kEntryUncached, MemoryType::eUncached);
    pat.setEntry(kEntryWriteCombined, MemoryType::eWriteCombine);
    pat.setEntry(kEntryWriteThrough, MemoryType::eWriteThrough);
    pat.setEntry(kEntryWriteBack, MemoryType::eWriteBack);
    pat.setEntry(kEntryWriteProtect, MemoryType::eWriteProtect);
    pat.setEntry(kEntryUncachedOverridable, MemoryType::eUncachedOverridable);

    // PAT[6] and PAT[7] are unused for now, so just set them to UC-
    pat.setEntry(6, MemoryType::eUncachedOverridable);
    pat.setEntry(7, MemoryType::eUncachedOverridable);

    for (uint8_t i = 0; i < pat.count(); i++) {
        km::MemoryType type = pat.getEntry(i);
        KmDebugMessage("[INIT] PAT[", i, "]: ", type, "\n");
    }

    return PageMemoryTypeLayout {
        .deferred = kEntryUncachedOverridable,
        .uncached = kEntryUncached,
        .writeCombined = kEntryWriteCombined,
        .writeThrough = kEntryWriteThrough,
        .writeProtect = kEntryWriteProtect,
        .writeBack = kEntryWriteBack,
    };
}

static void KmSetupMtrrs(const km::PageManager& pm) {
    if (!x64::HasMtrrSupport()) {
        return;
    }

    x64::MemoryTypeRanges mtrrs;

    KmDebugMessage("[INIT] MTRR fixed support: ", present(mtrrs.fixedMtrrSupported()), "\n");
    KmDebugMessage("[INIT] MTRR fixed enabled: ", enabled(mtrrs.fixedMtrrEnabled()), "\n");
    KmDebugMessage("[INIT] MTRR fixed count: ", mtrrs.fixedMtrrCount(), "\n");

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

                x64::FixedMtrr mtrr = mtrrs.fixedMtrr(i);
                KmDebugMessage(mtrr.type(), " ");
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
}

static void KmMigrateTerminal(const km::Canvas *display, SystemMemory& memory) {
    // migrate framebuffer memory
    KmMigrateMemory(memory.vmm, memory.pager, display->address(), display->size(), MemoryType::eWriteCombine);
}

static SystemMemory KmInitMemoryMap(uintptr_t bits, const KernelLaunch& launch) {
    PageMemoryTypeLayout pat = KmSetupPat();

    SystemMemory memory = SystemMemory { SystemMemoryLayout::from(launch.memoryMap), bits, launch.hhdmOffset, pat };

    KmSetupMtrrs(memory.pager);

    // initialize our own page tables and remap everything into it
    KmMapKernel(memory.pager, memory.vmm, memory.layout, launch.kernelPhysicalBase, launch.kernelVirtualBase);

    // move our stack out of reclaimable memory
    // limine garuntees 64k of stack space
    KmMigrateMemory(memory.vmm, memory.pager, (void*)(launch.stack.front.address + launch.hhdmOffset), launch.stack.size(), MemoryType::eUncached);

    // remap framebuffers
    for (const KernelFrameBuffer& framebuffer : launch.framebuffers) {
        KmMigrateMemory(memory.vmm, memory.pager, (void*)(framebuffer.address.address + launch.hhdmOffset), framebuffer.size(), MemoryType::eWriteCombine);
    }

    // once it is safe to remap the boot memory, do so
    KmReclaimBootMemory(memory.pager, memory.vmm, memory.layout);

    return memory;
}

static LocalApic KmEnableAPIC(km::SystemMemory& memory, km::IsrAllocator& isrs) {
    LocalApic lapic = KmInitBspLocalApic(memory);
    gLocalApic = lapic;

    uint8_t spuriousVec = isrs.allocateIsr();
    KmDebugMessage("[INIT] APIC ID: ", lapic.id(), ", Version: ", lapic.version(), ", Spurious vector: ", spuriousVec, "\n");

    KmInstallIsrHandler(spuriousVec, [](km::IsrContext *ctx) -> void* {
        KmDebugMessage("[ISR] Spurious interrupt: ", ctx->vector, "\n");
        gLocalApic.clearEndOfInterrupt();
        return ctx;
    });

    lapic.setSpuriousVector(spuriousVec);

    for (apic::Ivt ivt : {apic::Ivt::eTimer, apic::Ivt::eThermal, apic::Ivt::ePerformance, apic::Ivt::eLvt0, apic::Ivt::eLvt1, apic::Ivt::eError}) {
        lapic.configure(ivt, { .enabled = false });
    }

    lapic.enable();

    return lapic;
}

static km::PhysicalAddress KmGetRSDPTable(const KernelLaunch& launch) {
    return launch.rsdpAddress;
}

static void KmGetTerminal(const KernelLaunch& launch, SystemMemory& memory) {
    for (const KernelFrameBuffer& framebuffer : launch.framebuffers) {
        if (framebuffer.address == nullptr)
            continue;

        km::Canvas display { framebuffer, (uint8_t*)(framebuffer.address.address + launch.hhdmOffset) };
        KmMigrateTerminal(&display, memory);

        gTerminalLog = TerminalLog(display, memory);
        gLogTargets.add(&gTerminalLog);

        return;
    }
}

static void KmUpdateSerialPort(ComPortInfo info) {
    if (OpenSerialResult com1 = openSerial(info); com1.status == SerialPortStatus::eOk) {
        gSerialLog = SerialLog(com1.port);
        gLogTargets.add(&gSerialLog);
    }
}

static SerialPortStatus KmInitSerialPort(ComPortInfo info) {
    if (OpenSerialResult com1 = openSerial(info)) {
        return com1.status;
    } else {
        gSerialLog = SerialLog(com1.port);
        gLogTargets.add(&gSerialLog);
        return SerialPortStatus::eOk;
    }
}

static km::SystemMemory *gMemoryMap = nullptr;

[[noreturn]]
static void KmDumpIsrContext(const km::IsrContext *context, stdx::StringView message) {
    if (gMemoryMap != nullptr) {
        km::LocalApic lapic = km::LocalApic::current(*gMemoryMap);

        KmDebugMessage("\n[BUG] ", message, " - On core ", lapic.id(), "\n");
    } else {
        KmDebugMessage("\n[BUG] ", message, "\n");
    }

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

    KM_PANIC("Kernel panic.");
}

static void KmInstallExceptionHandlers(void) {
    KmInstallIsrHandler(0x0, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "Divide by zero (#DE)");
    });

    KmInstallIsrHandler(0x6, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "Invalid opcode (#UD)");
    });

    KmInstallIsrHandler(0x8, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "Double fault (#DF)");
    });

    KmInstallIsrHandler(0xD, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "General protection fault (#GP)");
    });

    KmInstallIsrHandler(0xE, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "Page fault (#PF)");
    });
}

struct [[gnu::packed]] alignas(0x10) TaskStateSegment {
    uint8_t reserved0[4];
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint8_t reserved1[8];
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint8_t reserved2[8];
    uint32_t iopbOffset;
};

static void KmWriteMemoryMap(const KernelMemoryMap& memmap, const SystemMemory& memory) {
    KmDebugMessage("[INIT] ", memmap.count(), " memory map entries.\n");

    KmDebugMessage("| Entry | Address            | Size             | Type\n");
    KmDebugMessage("|-------+--------------------+------------------+-----------------------\n");

    for (ssize_t i = 0; i < memmap.count(); i++) {
        MemoryMapEntry entry = memmap[i];
        MemoryRange range = entry.range;

        KmDebugMessage("| ", Int(i).pad(4, '0'), "  | ", Hex(range.front.address).pad(16, '0'), " | ", Hex(range.size()).pad(16, '0'), " | ", entry.type, "\n");
    }

    uint64_t usableMemory = 0;
    for (const MemoryMapEntry& range : memory.layout.available) {
        usableMemory += range.range.size();
    }

    uint64_t reclaimableMemory = 0;
    for (const MemoryMapEntry& range : memory.layout.reclaimable) {
        reclaimableMemory += range.range.size();
    }

    KmDebugMessage("[INIT] Usable memory: ", sm::bytes(usableMemory), ", Reclaimable memory: ", sm::bytes(reclaimableMemory), "\n");
}

static void KmInitPortDelay() {
    KmSetPortDelayMethod(x64::PortDelay::ePostCode);
}

static ssize_t KmInitEarlyTerminal(const KernelLaunch& launch) {
    auto fb = launch.framebuffers[0];
    Canvas canvas { fb, (uint8_t*)(fb.address.address + launch.hhdmOffset) };
    gEarlyTerminalLog = EarlyTerminalLog(canvas);

    ssize_t index = gLogTargets.count();
    gLogTargets.add(&gEarlyTerminalLog);

    return index;
}

extern "C" void KmLaunch(KernelLaunch launch) {
    __cli();

    ssize_t earlyTerminalIndex = KmInitEarlyTerminal(launch);

    KmInitPortDelay();

    bool hvPresent = IsHypervisorPresent();
    HypervisorInfo hvInfo{};
    bool hasDebugPort = false;

    if (hvPresent) {
        hvInfo = KmGetHypervisorInfo();

        if (hvInfo.platformHasDebugPort()) {
            hasDebugPort = KmTestDebugPort();
        }
    }

    if (hasDebugPort) {
        gLogTargets.add(&gDebugPortLog);
    }

    ProcessorInfo processor = KmGetProcessorInfo();

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
        .skipLoopbackTest = false,
    };

    SerialPortStatus com1Status = KmInitSerialPort(com1Info);

    x64::Cr0 cr0 = x64::Cr0::load();
    cr0.set(x64::Cr0::WP);
    cr0.set(x64::Cr0::NE);
    x64::Cr0::store(cr0);

    KmSetupBspGdt();

    km::IsrAllocator isrs;
    KmInitInterrupts(isrs, SystemGdt::eLongModeCode * sizeof(uint64_t));
    KmInstallExceptionHandlers();
    __sti();

    SystemMemory memory = KmInitMemoryMap(processor.maxpaddr, launch);
    gMemoryMap = &memory;

    // remove the early init terminal for the final terminal
    gLogTargets.remove(earlyTerminalIndex);
    KmGetTerminal(launch, memory);

    PlatformInfo platform = KmGetPlatformInfo(launch, memory);

    // On Oracle VirtualBox the COM1 port is functional but fails the loopback test.
    // If we are running on VirtualBox, retry the serial port initialization without the loopback test.
    if (com1Status == SerialPortStatus::eLoopbackTestFailed && platform.isOracleVirtualBox()) {
        KmUpdateSerialPort(com1Info);
    }

    // save the base address early as its stored in bootloader
    // reclaimable memory, which is reclaimed before this data is used.
    km::PhysicalAddress rsdpBaseAddress = KmGetRSDPTable(launch);

    KmDebugMessage("[INIT] System report.\n");
    KmDebugMessage("| Component     | Property             | Status\n");
    KmDebugMessage("|---------------+----------------------+-------\n");

    if (hvPresent) {
        KmDebugMessage("| /SYS/HV       | Hypervisor           | ", hvInfo.name, "\n");
        KmDebugMessage("| /SYS/HV       | Max CPUID leaf       | ", Hex(hvInfo.maxleaf).pad(8, '0'), "\n");
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

    for (ssize_t i = 0; i < launch.framebuffers.count(); i++) {
        const KernelFrameBuffer& display = launch.framebuffers[i];
        KmDebugMessage("| /SYS/VIDEO", i, "   | Display resolution   | ", display.width, "x", display.height, "x", display.bpp, "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Framebuffer size     | ", sm::bytes(display.size()), "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Framebuffer address  | ", display.address, "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | Display pitch        | ", display.pitch, "\n");
        KmDebugMessage("| /SYS/VIDEO", i, "   | EDID                 | ", display.edid, "\n");
    }

    KmDebugMessage("| /SYS/MB/COM1  | Status               | ", com1Status, "\n");
    KmDebugMessage("| /SYS/MB/COM1  | Port                 | ", Hex(com1Info.port), "\n");
    KmDebugMessage("| /SYS/MB/COM1  | Baud rate            | ", km::com::kBaudRate / com1Info.divisor, "\n");

    KmWriteMemoryMap(launch.memoryMap, memory);

    // Test interrupt to ensure the IDT is working
    {
        KmIsrHandler isrHandler = KmInstallIsrHandler(0x1, [](km::IsrContext *context) -> void* {
            KmDebugMessage("[INT] Test interrupt.\n");
            return context;
        });

        __int<0x1>();

        KmInstallIsrHandler(0x1, isrHandler);
    }

    LocalApic lapic = KmEnableAPIC(memory, isrs);

    // test lapic ipis to ensure the local apic is working
    {
        KmIsrHandler isrHandler = [](km::IsrContext *context) -> void* {
            KmDebugMessage("[INT] APIC interrupt.\n");
            gLocalApic.clearEndOfInterrupt();
            return context;
        };

        uint8_t testVec = isrs.allocateIsr();

        KmIsrHandler oldHandler = KmInstallIsrHandler(testVec, isrHandler);

        // another test interrupt
        lapic.sendIpi(apic::IcrDeliver::eSelf, testVec);
        lapic.sendIpi(apic::IcrDeliver::eSelf, testVec);
        lapic.sendIpi(apic::IcrDeliver::eSelf, testVec);

        isrs.releaseIsr(testVec);

        KmInstallIsrHandler(testVec, oldHandler);
    }

    acpi::AcpiTables rsdt = KmInitAcpi(rsdpBaseAddress, memory);
    uint32_t ioApicCount = rsdt.ioApicCount();
    KM_CHECK(ioApicCount > 0, "No IOAPICs found.");

    for (uint32_t i = 0; i < ioApicCount; i++) {
        IoApic ioapic = rsdt.mapIoApic(memory, 0);

        KmDebugMessage("[INIT] IOAPIC ", i, " ID: ", ioapic.id(), ", Version: ", ioapic.version(), "\n");
        KmDebugMessage("[INIT] ISR base: ", ioapic.isrBase(), ", Inputs: ", ioapic.inputCount(), "\n");
    }

    KmInitSmp(memory, lapic, rsdt);

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
