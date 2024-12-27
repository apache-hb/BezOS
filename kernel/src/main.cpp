// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "apic.hpp"
#include "display.hpp"
#include "gdt.hpp"
#include "hypervisor.hpp"
#include "isr.hpp"
#include "memory.hpp"
#include "pat.hpp"
#include "uart.hpp"
#include "acpi.hpp"

#include "memory/layout.hpp"
#include "memory/allocator.hpp"

#include "arch/intrin.hpp"

#include "kernel.hpp"

#include "util/logger.hpp"

#include <limine.h>

using namespace km;
using namespace stdx::literals;

[[gnu::used, gnu::section(".limine_requests")]]
static volatile LIMINE_BASE_REVISION(3)

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_memmap_request gMemmoryMapRequest = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_kernel_address_request gExecutableAddressRequest = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_hhdm_request gDirectMapRequest = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_rsdp_request gAcpiTableRequest = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_framebuffer_request gFramebufferRequest = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests_start")]]
static volatile LIMINE_REQUESTS_START_MARKER

[[gnu::used, gnu::section(".limine_requests_end")]]
static volatile LIMINE_REQUESTS_END_MARKER

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

class NullLog final : public ILogTarget {
    void write(stdx::StringView) override { }
};

class DebugPortLog final : public ILogTarget {
    void write(stdx::StringView message) override {
        for (char c : message) {
            __outbyte(0xE9, c);
        }
    }
};

class TerminalLog final : public ILogTarget {
    Terminal mTerminal;

    ILogTarget *mNext;

public:
    constexpr TerminalLog()
        : mTerminal(sm::noinit{})
        , mNext(nullptr)
    { }

    constexpr TerminalLog(Terminal terminal, ILogTarget *next)
        : mTerminal(terminal)
        , mNext(next)
    { }

    void write(stdx::StringView message) override {
        mNext->write(message);
        mTerminal.print(message);
    }
};

constinit static NullLog gNullLog;
constinit static DebugPortLog gDebugPortLog;

// load bearing constinit, clang has a bug in c++26 mode
// where it doesnt emit a warning for global constructors in all cases.
constinit static SerialLog gSerialLog;
constinit static TerminalLog gTerminalLog;

constinit static ILogTarget *gLogTarget = &gNullLog;

constinit static LocalAPIC gLocalApic;

// qemu e9 port check - i think bochs does something else
static bool KmTestDebugPort(void) {
    return __inbyte(0xE9) == 0xE9;
}

void KmDebugWrite(stdx::StringView value) {
    gLogTarget->write(value);
}

enum GdtEntry {
    eGdtEntry_Null = 0,
    eGdtEntry_Ring0Code = 1,
    eGdtEntry_Ring0Data = 2,

    eGdtEntry_Count
};

using x64::DescriptorFlags;
using x64::SegmentAccessFlags;

static constexpr uint64_t kGdtEntries[eGdtEntry_Count] = {
    [eGdtEntry_Null] = 0,
    [eGdtEntry_Ring0Code] = KmBuildSegmentDescriptor(
        DescriptorFlags::eLong | DescriptorFlags::eGranularity,
        SegmentAccessFlags::eExecutable | SegmentAccessFlags::eCodeOrDataSegment | SegmentAccessFlags::eReadWrite | SegmentAccessFlags::eRing0 | SegmentAccessFlags::ePresent | SegmentAccessFlags::eAccessed,
        0
    ),
    [eGdtEntry_Ring0Data] = KmBuildSegmentDescriptor(
        DescriptorFlags::eLong | DescriptorFlags::eGranularity,
        SegmentAccessFlags::eCodeOrDataSegment | SegmentAccessFlags::eRing0 | SegmentAccessFlags::eReadWrite | SegmentAccessFlags::ePresent | SegmentAccessFlags::eAccessed,
        0
    )
};

static void KmSetupGdt(void) {
    KmInitGdt(kGdtEntries, eGdtEntry_Count, eGdtEntry_Ring0Code, eGdtEntry_Ring0Data);
}

static SystemMemory KmInitMemoryMap(uintptr_t bits, const void *stack, const km::Display *display) {
    const limine_memmap_response *memmap = gMemmoryMapRequest.response;
    const limine_kernel_address_response *kernel = gExecutableAddressRequest.response;
    const limine_hhdm_response *hhdm = gDirectMapRequest.response;

    KM_CHECK(memmap != NULL, "No memory map");
    KM_CHECK(kernel != NULL, "No kernel address");
    KM_CHECK(hhdm != NULL, "No higher half direct map");

    if (x64::HasPatSupport()) {
        x64::PageAttributeTable pat;

        for (uint8_t i = 0; i < pat.count(); i++) {
            x64::MemoryType type = pat.getEntry(i);
            KmDebugMessage("[INIT] PAT[", i, "]: ", type, "\n");
        }
    }

    SystemMemory memory = SystemMemory { SystemMemoryLayout::from(*memmap), bits, *hhdm };

    // initialize our own page tables and remap everything into it
    KmMapKernel(memory.pager, memory.vmm, memory.layout, *kernel);

    // move our stack out of reclaimable memory
    // limine garuntees 64k of stack space
    KmMigrateMemory(memory.vmm, memory.pager, stack, 0x10000);

    // migrate framebuffer memory
    KmMigrateMemory(memory.vmm, memory.pager, display->address(), display->size());

    // once it is safe to remap the boot memory, do so
    KmReclaimBootMemory(memory.pager, memory.vmm, memory.layout);

    if (x64::HasMtrrSupport()) {
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
                KmDebugMessage("[INIT] Variable MTRR[", i, "]: ", mtrr.type(), ", address: ", mtrr.baseAddress(memory.pager), ", mask: ", Hex(mtrr.addressMask(memory.pager)).pad(16, '0'), "\n");
            }
        }
    }

    return memory;
}


static LocalAPIC KmEnableAPIC(km::VirtualAllocator& vmm, const km::PageManager& pm, km::IsrAllocator& isrs) {
    // disable the 8259 PIC first
    KmDisablePIC();

    LocalAPIC lapic = KmInitLocalAPIC(vmm, pm);
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

static km::PhysicalAddress KmGetRSDPTable(void) {
    const limine_rsdp_response *response = gAcpiTableRequest.response;
    KM_CHECK(response != NULL, "No RSDP table");

    return km::PhysicalAddress { (uintptr_t)response->address };
}

static km::Terminal KmGetTerminal(void) {
    const limine_framebuffer_response *response = gFramebufferRequest.response;
    KM_CHECK(response != NULL, "No framebuffer response");
    KM_CHECK(response->framebuffer_count > 0, "No framebuffer");

    km::Display display { *response->framebuffers[0] };
    return km::Terminal { display, uint16_t(display.height() / 8), uint16_t(display.width() / 8) };
}

static SerialPortStatus KmInitSerialPort(ComPortInfo info) {
    if (OpenSerialResult com1 = openSerial(info)) {
        return com1.status;
    } else {
        gSerialLog = SerialLog(com1.port);
        gLogTarget = &gSerialLog;
        return SerialPortStatus::eOk;
    }
}

[[noreturn]]
static void KmDumpIsrContext(const km::IsrContext *context, stdx::StringView message) {
    KmDebugMessage("\n[BUG] ", message, "\n");
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
        KmDumpIsrContext(context, "Divide by zero (#DE).");
    });

    KmInstallIsrHandler(0x6, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "Invalid opcode (#UD).");
    });

    KmInstallIsrHandler(0x8, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "Double fault (#DF).");
    });

    KmInstallIsrHandler(0xD, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "General protection fault (#GP).");
    });

    KmInstallIsrHandler(0xE, [](km::IsrContext *context) -> void* {
        KmDumpIsrContext(context, "Page fault (#PF).");
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

extern "C" void kmain(void) {
    __cli();

    // offset the stack pointer as limine pushes qword 0 to
    // the stack before jumping to the kernel. and builtin_frame_address
    // returns the address where call would store the return address.
    [[maybe_unused]]
    void *base = (char*)__builtin_frame_address(0) + (sizeof(void*) * 2);

    bool hvPresent = KmIsHypervisorPresent();
    HypervisorInfo hvInfo;
    bool hasDebugPort = false;

    if (hvPresent) {
        hvInfo = KmGetHypervisorInfo();

        if (hvInfo.platformHasDebugPort()) {
            hasDebugPort = KmTestDebugPort();
        }
    }

    if (hasDebugPort) {
        gLogTarget = &gDebugPortLog;
    }

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600
    };

    SerialPortStatus com1Status = KmInitSerialPort(com1Info);

    KM_CHECK(LIMINE_BASE_REVISION_SUPPORTED, "Unsupported limine base revision.");

    ProcessorInfo processor = KmGetProcessorInfo();

    // save the base address early as its stored in bootloader
    // reclaimable memory, which is reclaimed before this data is used.
    km::PhysicalAddress rsdpBaseAddress = KmGetRSDPTable();

    km::Terminal terminal = KmGetTerminal();
    km::Display& display = terminal.display();
    // display.fill(Pixel { 0, 0, 0 });

    gTerminalLog = TerminalLog(terminal, gLogTarget);
    gLogTarget = &gTerminalLog;

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

    KmDebugMessage("| /SYS/MB/COM1  | Status               | ", com1Status, "\n");
    KmDebugMessage("| /SYS/MB/COM1  | Port                 | ", Hex(com1Info.port), "\n");
    KmDebugMessage("| /SYS/MB/COM1  | Baud rate            | ", km::com::kBaudRate / com1Info.divisor, "\n");

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

    KmDebugMessage("| /SYS/VIDEO    | Display size         | ", display.width(), "x", display.height(), "x", display.bpp(), "\n");
    KmDebugMessage("| /SYS/VIDEO    | Display address      | ", Hex((uintptr_t)display.address()).pad(16, '0'), "\n");
    KmDebugMessage("| /SYS/VIDEO    | Display pitch        | ", display.pitch(), "\n");

    KmSetupGdt();

    km::IsrAllocator isrs;
    KmInitInterrupts(isrs, eGdtEntry_Ring0Code * sizeof(uint64_t));
    KmInstallExceptionHandlers();
    __sti();

    // reclaims bootloader memory, all the limine requests must be read
    // before this point.

    SystemMemory memory = KmInitMemoryMap(processor.maxpaddr, base, &display);

    // test interrupt to ensure the IDT is working
    {
        KmIsrHandler isrHandler = KmInstallIsrHandler(0x1, [](km::IsrContext *context) -> void* {
            KmDebugMessage("[INT] Test interrupt.\n");
            return context;
        });

        __int<0x1>();

        KmInstallIsrHandler(0x1, isrHandler);
    }

    LocalAPIC lapic = KmEnableAPIC(memory.vmm, memory.pager, isrs);

    // test lapic ipis to ensure the local apic is working
    {
        KmIsrHandler testHandler = [](km::IsrContext *context) -> void* {
            KmDebugMessage("[INT] APIC interrupt.\n");
            gLocalApic.clearEndOfInterrupt();
            return context;
        };

        uint8_t testVec = isrs.allocateIsr();

        KmIsrHandler oldHandler = KmInstallIsrHandler(testVec, testHandler);

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

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
