// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "memory/virtual.hpp"
#include "std/static_string.hpp"

#include "arch/intrin.hpp"
#include "arch/paging.hpp"

#include "kernel.hpp"

#include "util/cpuid.hpp"
#include "util/memory.hpp"

#include "memory/physical.hpp"

#include <limine.h>

using namespace km;

struct PhysicalAddress {
    uint64_t address;
};

struct VirtualAddress {
    uint64_t address;
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile LIMINE_BASE_REVISION(3)

[[gnu::used, gnu::section(".limine_requests")]]
static volatile struct limine_memmap_request gMemmoryMapRequest = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile struct limine_kernel_address_request gExecutableAddressRequest = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests_start")]]
static volatile LIMINE_REQUESTS_START_MARKER

[[gnu::used, gnu::section(".limine_requests_end")]]
static volatile LIMINE_REQUESTS_END_MARKER

static bool gHasDebugPort = false;

// qemu e9 port check - i think bochs does something else
static bool HasDebugPort(void) {
    return gHasDebugPort;
}

static bool KmTestDebugPort(void) {
    return __inbyte(0xE9) == 0xE9;
}

void KmDebugWrite(stdx::StringView value) {
    if (!HasDebugPort())
        return;

    for (char c : value) {
        __outbyte(0xE9, c);
    }
}

enum DescriptorFlags {
    eDescriptorFlag_Long = (1 << 1),
    eDescriptorFlag_Size = (1 << 2),
    eDescriptorFlag_Granularity = (1 << 3),
};

enum SegmentAccessFlags {
    eSegmentAccess_Accessed = (1 << 0),
    eSegmentAccess_ReadWrite = (1 << 1),
    eSegmentAccess_EscalateDirection = (1 << 2),
    eSegmentAccess_Executable = (1 << 3),
    eSegmentAccess_CodeOrDataSegment = (1 << 4),

    eSegmentAccess_Ring0 = (0 << 5),
    eSegmentAccess_Ring1 = (1 << 5),
    eSegmentAccess_Ring2 = (2 << 5),
    eSegmentAccess_Ring3 = (3 << 5),

    eSegmentAccess_Present = (1 << 7),
};

static constexpr uint64_t BuildSegmentDescriptor(DescriptorFlags flags, SegmentAccessFlags access, uint16_t limit) {
    return (((uint64_t)(flags) << 52) | ((uint64_t)(access) << 40)) | (0xFULL << 48) | (uint16_t)limit;
}

enum GdtEntry {
    eGdtEntry_Null = 0,
    eGdtEntry_Ring0Code = 1,
    eGdtEntry_Ring0Data = 2,

    eGdtEntry_Count
};

static const uint64_t gGdtEntries[eGdtEntry_Count] = {
    [eGdtEntry_Null] = 0,
    [eGdtEntry_Ring0Code] = BuildSegmentDescriptor(
        DescriptorFlags(eDescriptorFlag_Long | eDescriptorFlag_Granularity),
        SegmentAccessFlags(eSegmentAccess_Executable | eSegmentAccess_CodeOrDataSegment | eSegmentAccess_ReadWrite | eSegmentAccess_Ring0 | eSegmentAccess_Present | eSegmentAccess_Accessed),
        0
    ),
    [eGdtEntry_Ring0Data] = BuildSegmentDescriptor(
        DescriptorFlags(eDescriptorFlag_Long | eDescriptorFlag_Granularity),
        SegmentAccessFlags(eSegmentAccess_CodeOrDataSegment | eSegmentAccess_Ring0 | eSegmentAccess_ReadWrite | eSegmentAccess_Present | eSegmentAccess_Accessed),
        0
    )
};

static void KmInitGdt(void) {
    struct GDTR gdtr = {
        .limit = sizeof(gGdtEntries) - 1,
        .base = (uint64_t)gGdtEntries
    };

    __lgdt(gdtr);

    uint64_t codeSegment = eGdtEntry_Ring0Code * sizeof(uint64_t);
    asm volatile (
        // long jump to reload the CS register with the new code segment
        "pushq %[code]\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:"
        : /* no outputs */
        : [code] "r"(codeSegment)
        : "memory", "rax"
    );

    uint16_t dataSegment = eGdtEntry_Ring0Data * sizeof(uint64_t);
    asm volatile (
        // zero out all segments aside from ss
        "mov $0, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        // load the data segment into ss
        "mov %[data], %%ax\n"
        "mov %%ax, %%ss\n"
        : /* no outputs */
        : [data] "r"(dataSegment) /* inputs */
        : "memory", "rax" /* clobbers */
    );
}

struct SystemMemory {
    SystemMemoryLayout mLayout;
    PhysicalAllocator mPhysicalAllocator;
    PageAllocator pPageAllocator;

    SystemMemory(SystemMemoryLayout layout)
        : mLayout(layout)
        , mPhysicalAllocator(&mLayout)
        , pPageAllocator(&mPhysicalAllocator)
    { }
};

static SystemMemory KmInitMemoryMap(void) {
    const struct limine_memmap_response *memmap = gMemmoryMapRequest.response;
    KM_CHECK(memmap != NULL, "No memory map!");

    return SystemMemory { SystemMemoryLayout::from(*gMemmoryMapRequest.response) };
}

static bool IsHypervisorPresent(void) {
    static constexpr uint32_t kHypervisorBit = (1 << 31);

    km::CpuId cpuid = km::CpuId::of(1);
    return cpuid.ecx & kHypervisorBit;
}

struct HypervisorInfo {
    stdx::StaticString<12> name;
    uint32_t maxleaf;

    bool isKvm(void) const noexcept {
        return name == "KVMKVMKVM\0\0\0";
    }

    bool isQemu(void) const noexcept {
        return name == "TCGTCGTCGTCG" || name == "KVMKVMKVM\0\0\0";
    }

    bool isMicrosoftHyperV(void) const noexcept {
        return name == "Microsoft Hv";
    }

    bool platformHasDebugPort(void) const noexcept {
        return isQemu();
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

    CoreMultiplier coreClock;
    uint32_t busClock; // in hz

    bool hasNominalFrequency() const noexcept {
        return busClock != 0;
    }

    uint16_t baseFrequency; // in mhz
    uint16_t maxFrequency; // in mhz
    uint16_t busFrequency; // in mhz
};

static BrandString KmGetBrandString() noexcept {
    char brand[km::kBrandStringSize];
    km::GetBrandString(brand);
    return brand;
}

static uintptr_t KmGetMaxPhysicalAddress() noexcept {
    km::CpuId cpuid = km::CpuId::of(0x80000008);
    return cpuid.eax & 0xFF;
}

static ProcessorInfo KmGetProcessorInfo() {
    km::CpuId vendorId = km::CpuId::of(0);

    char vendor[12];
    memcpy(vendor + 0, &vendorId.ebx, 4);
    memcpy(vendor + 4, &vendorId.edx, 4);
    memcpy(vendor + 8, &vendorId.ecx, 4);
    uint32_t maxleaf = vendorId.eax;

    CpuId ext = CpuId::of(0x80000000);
    BrandString brand = ext.eax < 0x80000004 ? "" : KmGetBrandString();

    uintptr_t maxpaddr = ext.eax < 0x80000008 ? 0 : KmGetMaxPhysicalAddress();

    if (maxleaf < 0x16) {
        return ProcessorInfo { vendor, brand, maxleaf, maxpaddr };
    }

    km::CpuId tsc = km::CpuId::of(0x15);

    CoreMultiplier coreClock = {
        .tsc = tsc.eax,
        .core = tsc.ebx
    };

    uint32_t busClock = tsc.ecx;

    km::CpuId frequency = km::CpuId::of(0x16);

    return ProcessorInfo {
        .vendor = vendor,
        .brand = brand,
        .maxleaf = maxleaf,
        .maxpaddr = maxpaddr,
        .coreClock = coreClock,
        .busClock = busClock,
        .baseFrequency = uint16_t(frequency.eax & 0xFFFF),
        .maxFrequency = uint16_t(frequency.ebx & 0xFFFF),
        .busFrequency = uint16_t(frequency.ecx & 0xFFFF),
    };
}

extern "C" void kmain(void) {
    __cli();

    bool hvPresent = IsHypervisorPresent();
    HypervisorInfo hvInfo;
    if (hvPresent) {
        hvInfo = KmGetHypervisorInfo();

        if (hvInfo.platformHasDebugPort()) {
            gHasDebugPort = KmTestDebugPort();
        }
    }

    KM_CHECK(LIMINE_BASE_REVISION_SUPPORTED, "Unsupported limine base revision.");

    ProcessorInfo processor = KmGetProcessorInfo();

    if (processor.maxpaddr != x64::paging::kMaxPhysicalAddress) {
        KmDebugMessage("[INIT] Unsupported physical address size ", processor.maxpaddr, ". Only", x64::paging::kMaxPhysicalAddress, " bit addressing is supported.\n");
        KM_PANIC("Unsupported physical address size.");
    }

    KmDebugMessage("[INIT] System report.\n");
    KmDebugMessage("| Component     | Property             | Status\n");
    KmDebugMessage("|---------------+----------------------+-------\n");

    if (hvPresent) {
        KmDebugMessage("| /SYS/HV       | Hypervisor           | ", hvInfo.name, "\n");
        KmDebugMessage("| /SYS/HV       | Max CPUID leaf       | ", Hex(hvInfo.maxleaf).pad(8, '0'), "\n");
        KmDebugMessage("| /SYS/HV       | e9 debug port        | ", gHasDebugPort ? stdx::StringView("Enabled") : stdx::StringView("Disabled"), "\n");
    } else {
        KmDebugMessage("| /SYS/HV       | Hypervisor           | Not present\n");
        KmDebugMessage("| /SYS/HV       | Max CPUID leaf       | 0x00000000\n");
        KmDebugMessage("| /SYS/HV       | e9 debug port        | Not applicable\n");
    }

    KmDebugMessage("| /SYS/MB/CPU0  | Vendor               | ", processor.vendor, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Model name           | ", processor.brand, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Max CPUID leaf       | ", Hex(processor.maxleaf).pad(8, '0'), "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Max physical address | ", processor.maxpaddr, "\n");
    KmDebugMessage("| /SYS/MB/CPU0  | TSC ratio            | ", processor.coreClock.tsc, "/", processor.coreClock.core, "\n");

    if (processor.hasNominalFrequency()) {
        KmDebugMessage("| /SYS/MB/CPU0  | Bus clock            | ", processor.busClock, "hz\n");
    } else {
        KmDebugMessage("| /SYS/MB/CPU0  | Bus clock            | Not available\n");
    }

    KmDebugMessage("| /SYS/MB/CPU0  | Base frequency       | ", processor.baseFrequency, "mhz\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Max frequency        | ", processor.maxFrequency, "mhz\n");
    KmDebugMessage("| /SYS/MB/CPU0  | Bus frequency        | ", processor.busFrequency, "mhz\n");

    KmDebugMessage("[INIT] Load GDT.\n");
    KmInitGdt();

    [[maybe_unused]]
    SystemMemory layout = KmInitMemoryMap();

    KM_PANIC("Test bugcheck.");

    KmHalt();
}
