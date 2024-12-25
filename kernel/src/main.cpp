// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "apic.hpp"
#include "gdt.hpp"
#include "isr.hpp"
#include "memory.hpp"
#include "uart.hpp"
#include "acpi.hpp"

#include "memory/layout.hpp"
#include "memory/allocator.hpp"

#include "std/static_string.hpp"

#include "arch/intrin.hpp"
#include "arch/paging.hpp"

#include "kernel.hpp"

#include "util/cpuid.hpp"
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

constinit static NullLog gNullLog;
constinit static DebugPortLog gDebugPortLog;

// load bearing constinit, clang has a bug in c++26 mode
// where it doesnt emit a warning for global constructors in all cases.
constinit static SerialLog gSerialLog;

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

void KmSetupGdt(void) {
    KmInitGdt(kGdtEntries, eGdtEntry_Count, eGdtEntry_Ring0Code, eGdtEntry_Ring0Data);
}

static SystemMemory KmInitMemoryMap(uintptr_t bits) {
    const limine_memmap_response *memmap = gMemmoryMapRequest.response;
    const limine_kernel_address_response *kernel = gExecutableAddressRequest.response;
    const limine_hhdm_response *hhdm = gDirectMapRequest.response;

    KM_CHECK(memmap != NULL, "No memory map!");
    KM_CHECK(kernel != NULL, "No kernel address!");
    KM_CHECK(hhdm != NULL, "No higher half direct map!");

    SystemMemory memory = SystemMemory { SystemMemoryLayout::from(*memmap), bits, *hhdm };
    KmMapKernel(memory.pager, memory.vmm, memory.layout, *kernel);
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

    for (auto ivt : {apic::Ivt::eTimer, apic::Ivt::eThermal, apic::Ivt::ePerformance, apic::Ivt::eLvt0, apic::Ivt::eLvt1, apic::Ivt::eError}) {
        lapic.configure(ivt, { .enabled = false });
    }

    lapic.enable();

    return lapic;
}

static km::PhysicalAddress KmGetRSDPTable(void) {
    const limine_rsdp_response *response = gAcpiTableRequest.response;
    KM_CHECK(response != NULL, "No RSDP table!");

    return km::PhysicalAddress { (uintptr_t)response->address };
}

static bool KmIsHypervisorPresent(void) {
    static constexpr uint32_t kHypervisorBit = (1 << 31);

    km::CpuId cpuid = km::CpuId::of(1);
    return cpuid.ecx & kHypervisorBit;
}

struct HypervisorInfo {
    stdx::StaticString<12> name;
    uint32_t maxleaf;

    bool isKvm(void) const {
        return name == "KVMKVMKVM\0\0\0"_sv;
    }

    bool isQemu(void) const {
        return name == "TCGTCGTCGTCG"_sv;
    }

    bool isMicrosoftHyperV(void) const {
        return name == "Microsoft Hv"_sv;
    }

    bool platformHasDebugPort(void) const {
        // qemu may use kvm, so check the e9 port in either case
        return isQemu() || isKvm();
    }
};

/// @pre: IsHypervisorPresent() = true
static HypervisorInfo KmGetHypervisorInfo(void) {
    km::CpuId cpuid = km::CpuId::of(0x40000000);

    char vendor[12];
    memcpy(vendor + 0, &cpuid.ebx, 4);
    memcpy(vendor + 4, &cpuid.ecx, 4);
    memcpy(vendor + 8, &cpuid.edx, 4);

    return HypervisorInfo { vendor, cpuid.eax };
}

// TODO: find a good units library
//       and try to make fmtlib build freestanding

struct CoreMultiplier {
    uint32_t tsc;
    uint32_t core;
};

using VendorString = stdx::StaticString<12>;
using BrandString = stdx::StaticString<48>;

struct ProcessorInfo {
    VendorString vendor;
    BrandString brand;
    uint32_t maxleaf;
    uintptr_t maxpaddr;
    uintptr_t maxvaddr;

    bool hasLocalApic;
    bool has2xApic;

    CoreMultiplier coreClock;
    uint32_t busClock; // in hz

    bool hasNominalFrequency() const {
        return busClock != 0;
    }

    uint16_t baseFrequency; // in mhz
    uint16_t maxFrequency; // in mhz
    uint16_t busFrequency; // in mhz
};

static BrandString KmGetBrandString() {
    char brand[km::kBrandStringSize];
    km::KmGetBrandString(brand);
    return brand;
}

static ProcessorInfo KmGetProcessorInfo() {
    km::CpuId vendorId = km::CpuId::of(0);

    char vendor[12];
    memcpy(vendor + 0, &vendorId.ebx, 4);
    memcpy(vendor + 4, &vendorId.edx, 4);
    memcpy(vendor + 8, &vendorId.ecx, 4);
    uint32_t maxleaf = vendorId.eax;

    km::CpuId cpuid = km::CpuId::of(1);
    bool isLocalApicPresent = cpuid.edx & (1 << 9);
    bool is2xApicPresent = cpuid.ecx & (1 << 21);

    CpuId ext = CpuId::of(0x80000000);
    BrandString brand = ext.eax < 0x80000004 ? "" : KmGetBrandString();

    uintptr_t maxvaddr = 0;
    uintptr_t maxpaddr = 0;
    if (ext.eax >= 0x80000008) {
        km::CpuId leaf = km::CpuId::of(0x80000008);
        maxvaddr = (leaf.eax >> 8) & 0xFF;
        maxpaddr = (leaf.eax >> 0) & 0xFF;
    }

    CoreMultiplier coreClock = { 0, 0 };
    uint32_t busClock = 0;
    uint16_t baseFrequency = 0;
    uint16_t maxFrequency = 0;
    uint16_t busFrequency = 0;

    if (maxleaf >= 0x15) {
        km::CpuId tsc = km::CpuId::of(0x15);

        coreClock = {
            .tsc = tsc.eax,
            .core = tsc.ebx
        };

        busClock = tsc.ecx;
    }

    if (maxleaf >= 0x16) {
        km::CpuId frequency = km::CpuId::of(0x16);
        baseFrequency = frequency.eax & 0xFFFF;
        maxFrequency = frequency.ebx & 0xFFFF;
        busFrequency = frequency.ecx & 0xFFFF;
    }

    return ProcessorInfo {
        .vendor = vendor,
        .brand = brand,
        .maxleaf = maxleaf,
        .maxpaddr = maxpaddr,
        .maxvaddr = maxvaddr,
        .hasLocalApic = isLocalApicPresent,
        .has2xApic = is2xApicPresent,
        .coreClock = coreClock,
        .busClock = busClock,
        .baseFrequency = baseFrequency,
        .maxFrequency = maxFrequency,
        .busFrequency = busFrequency
    };
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

static void KmInstallExceptionHandlers(void) {
    KmInstallIsrHandler(0x0, [](km::IsrContext *context) -> void* {
        KM_PANIC("[INT] Divide by zero.\n");
        return context;
    });

    KmInstallIsrHandler(0x6, [](km::IsrContext *context) -> void* {
        KM_PANIC("[INT] Invalid opcode.\n");
        return context;
    });

    KmInstallIsrHandler(0x8, [](km::IsrContext *context) -> void* {
        KM_PANIC("[INT] Double fault.\n");
        return context;
    });

    KmInstallIsrHandler(0xD, [](km::IsrContext *context) -> void* {
        KM_PANIC("[INT] General protection fault.\n");
        return context;
    });

    KmInstallIsrHandler(0xE, [](km::IsrContext *context) -> void* {
        KM_PANIC("[INT] Page fault.\n");
        return context;
    });
}

extern "C" void kmain(void) {
    __cli();

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

    if (!x64::paging::isValidAddressWidth(processor.maxpaddr)) {
        KmDebugMessage("[INIT] Unsupported physical address width ", processor.maxpaddr, "bits. Must be 40 or 48.\n");
        KM_PANIC("Unsupported physical address width.");
    }

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
    KmDebugMessage("| /SYS/MB/COM1  | Port                 | ", com1Info.port, "\n");
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

    KmSetupGdt();

    km::IsrAllocator isrs;
    KmInitInterrupts(isrs, eGdtEntry_Ring0Code * sizeof(uint64_t));
    KmInstallExceptionHandlers();
    __sti();

    // reclaims bootloader memory, all the limine requests must be read
    // before this point.
    SystemMemory memory = KmInitMemoryMap(processor.maxpaddr);

    KmIsrHandler isrHandler = KmInstallIsrHandler(0x1, [](km::IsrContext *context) -> void* {
        KmDebugMessage("[INT] Test interrupt.\n");
        return context;
    });

    // do a test interrupt
    __int<0x1>();

    KmInstallIsrHandler(0x1, isrHandler);

    LocalAPIC lapic = KmEnableAPIC(memory.vmm, memory.pager, isrs);

    KmIsrHandler testHandler = [](km::IsrContext *context) -> void* {
        KmDebugMessage("[INT] APIC interrupt.\n");
        gLocalApic.clearEndOfInterrupt();
        return context;
    };

    KmInstallIsrHandler(33, testHandler);

    // another test interrupt
    lapic.sendIpi(apic::IcrDeliver::eSelf, 33);
    lapic.sendIpi(apic::IcrDeliver::eSelf, 33);
    lapic.sendIpi(apic::IcrDeliver::eSelf, 33);

    acpi::AcpiTables rsdt = KmInitAcpi(rsdpBaseAddress, memory);
    IoApic ioapic = rsdt.mapIoApic(memory);

    KmDebugMessage("[INIT] ISR base: ", ioapic.isrBase(), ", ID: ", ioapic.id(), "\n");
    KmDebugMessage("[INIT] IOAPIC version: ", ioapic.version(), ", Inputs: ", ioapic.inputCount(), "\n");

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
