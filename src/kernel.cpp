// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#include "kernel.h"
#include "crt.h"
#include "arch.h"

#include <limine.h>
#include <x86intrin.h>
#include <cpuid.h>

#define __DEFAULT_FN_ATTRS __attribute__((__always_inline__, __nodebug__, unused))

struct [[gnu::packed]] GDTR {
    uint16_t limit;
    uint64_t base;
};

struct PhysicalAddress {
    uint64_t address;
};

struct VirtualAddress {
    uint64_t address;
};

static inline void __DEFAULT_FN_ATTRS __lgdt(struct GDTR gdtr) {
    asm volatile("lgdt %0" :: "m"(gdtr));
}

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3)

#if 0
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};
#endif

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER

static bool gHasDebugPort = false;

// qemu e9 port check - i think bochs does something else
static bool HasDebugPort(void) {
    return gHasDebugPort;
}

static void InitDebugPort(void) {
    gHasDebugPort = __inbyte(0xE9) == 0xE9;
}

void KmDebugMessage(const char *message) {
    if (!HasDebugPort())
        return;

    while (*message) {
        __outbyte(0xE9, *message++);
    }
}

template<typename T>
static void e9print_hex(T val) {
    if (!HasDebugPort())
        return;

    KmDebugMessage("0x");
    const char *hex = "0123456789ABCDEF";
    for (int i = (sizeof(T) * CHAR_BIT) - 4; i >= 0; i -= 4) {
        __outbyte(0xE9, hex[(val >> i) & 0xF]);
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

static uint64_t BuildSegmentDescriptor(DescriptorFlags flags, SegmentAccessFlags access, uint16_t limit) {
    return (((uint64_t)(flags) << 52) | ((uint64_t)(access) << 40)) | (0xFULL << 48) | (uint16_t)limit;
}

enum GdtEntry {
    eGdtEntry_Null = 0,
    eGdtEntry_Ring0Code = 1,
    eGdtEntry_Ring0Data = 2,

    eGdtEntry_Count
};

static uint64_t gGdtEntries[eGdtEntry_Count];

static void KmInitGdt(void) {
    gGdtEntries[eGdtEntry_Null] = 0;

    const DescriptorFlags kCodeFlags = DescriptorFlags(eDescriptorFlag_Long | eDescriptorFlag_Granularity);
    const SegmentAccessFlags kCodeAccess = SegmentAccessFlags(eSegmentAccess_Executable | eSegmentAccess_CodeOrDataSegment | eSegmentAccess_ReadWrite | eSegmentAccess_Ring0 | eSegmentAccess_Present | eSegmentAccess_Accessed);
    gGdtEntries[eGdtEntry_Ring0Code] = BuildSegmentDescriptor(kCodeFlags, kCodeAccess, 0);

    const DescriptorFlags kDataFlags = DescriptorFlags(eDescriptorFlag_Long | eDescriptorFlag_Granularity);
    const SegmentAccessFlags kDataAccess = SegmentAccessFlags(eSegmentAccess_CodeOrDataSegment | eSegmentAccess_Ring0 | eSegmentAccess_ReadWrite | eSegmentAccess_Present | eSegmentAccess_Accessed);
    gGdtEntries[eGdtEntry_Ring0Data] = BuildSegmentDescriptor(kDataFlags, kDataAccess, 0);

    struct GDTR gdtr = {
        .limit = sizeof(gGdtEntries) - 1,
        .base = (uint64_t)&gGdtEntries[0]
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

static const char *GetMemoryMapTypeName(uint64_t type) {
    static const char *const kMemoryMapTypeNames[] = {
        [LIMINE_MEMMAP_USABLE] = "Usable",
        [LIMINE_MEMMAP_RESERVED] = "Reserved",
        [LIMINE_MEMMAP_ACPI_RECLAIMABLE] = "ACPI Reclaimable",
        [LIMINE_MEMMAP_ACPI_NVS] = "ACPI NVS",
        [LIMINE_MEMMAP_BAD_MEMORY] = "Bad Memory",
        [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = "Bootloader Reclaimable",
        [LIMINE_MEMMAP_KERNEL_AND_MODULES] = "Kernel and Modules",
        [LIMINE_MEMMAP_FRAMEBUFFER] = "Framebuffer",
    };

    if (type < countof(kMemoryMapTypeNames)) {
        return kMemoryMapTypeNames[type];
    }
    return "Unknown";
}

static void KmInitPaging(void) {
    struct limine_memmap_response *memmap = memmap_request.response;
    KM_CHECK(memmap != NULL, "No memory map!");

    uint64_t usableMemory = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry == NULL) {
            KmDebugMessage("Memory map entry is NULL!\n");
            continue;
        }

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            usableMemory += entry->length;
        }

        KmDebugMessage("Memory map entry: ");
        e9print_hex(entry->base);
        KmDebugMessage(" ");
        e9print_hex(entry->length);
        KmDebugMessage(" ");
        KmDebugMessage(GetMemoryMapTypeName(entry->type));
        KmDebugMessage("\n");
    }

    KmDebugMessage("Usable memory: ");
    e9print_hex(usableMemory);
    KmDebugMessage("\n");
}

static bool IsHypervisorPresent(void) {
    static constexpr uint32_t kHypervisorBit = (1 << 31);
    uint32_t eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);

    return ecx & kHypervisorBit;
}

struct HypervisorInfo {
    char name[12 + 1];
    uint32_t maxleaf;

    bool SupportsPortE9(void) const noexcept {
        return memcmp(name, "TCGTCGTCGTCG", 12) == 0;
    }
};

/// @pre: IsHypervisorPresent() = true
static HypervisorInfo GetHypervisorInfo(void) {
    uint32_t eax, ebx, ecx, edx;
    __cpuid(0x40000000, eax, ebx, ecx, edx);

    HypervisorInfo info = {};
    memset(info.name, 0, sizeof(info.name));
    memcpy(info.name + 0, &ebx, 4);
    memcpy(info.name + 4, &ecx, 4);
    memcpy(info.name + 8, &edx, 4);
    info.maxleaf = eax;

    return info;
}

struct VendorInfo {
    char vendor[12 + 1];
    uint32_t maxleaf;
};

static VendorInfo GetVendorInfo(void) {
    uint32_t eax, ebx, ecx, edx;
    __cpuid(0, eax, ebx, ecx, edx);

    VendorInfo info = {};
    memset(info.vendor, 0, sizeof(info.vendor));
    memcpy(info.vendor + 0, &ebx, 4);
    memcpy(info.vendor + 4, &edx, 4);
    memcpy(info.vendor + 8, &ecx, 4);

    info.maxleaf = eax;

    return info;
}

extern "C" void kmain(void) {
    __cli();

    if (IsHypervisorPresent()) {
        HypervisorInfo hypervisorInfo = GetHypervisorInfo();

        InitDebugPort();

        KmDebugMessage("Hypervisor: ");
        KmDebugMessage(hypervisorInfo.name);
        KmDebugMessage("\n");
        KmDebugMessage("Highest CPUID leaf: ");
        e9print_hex(hypervisorInfo.maxleaf);
        KmDebugMessage("\n");
    }

    // Do this check as early as feasible, ideally after setting up the debug port
    // so we can see the message.
    // Ensure the bootloader actually understands our base revision (see spec).
    KM_CHECK(LIMINE_BASE_REVISION_SUPPORTED, "Unsupported base revision.");

    VendorInfo vendorInfo = GetVendorInfo();

    KmDebugMessage("Highest CPUID leaf: ");
    e9print_hex(vendorInfo.maxleaf);
    KmDebugMessage("\n");
    KmDebugMessage("Vendor: ");
    KmDebugMessage(vendorInfo.vendor);
    KmDebugMessage("\n");

    KmDebugMessage("[INIT] GDT.\n");
    KmInitGdt();

    KmDebugMessage("[INIT] Paging.\n");
    KmInitPaging();

    KM_BUGCHECK("Test bugcheck.");

#if 0
    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    volatile uint32_t *fb_ptr = framebuffer->address;

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    for (uint64_t x = 0; x < framebuffer->width; x++) {
        for (uint64_t y = 0; y < framebuffer->height; y++) {
            fb_ptr[x * framebuffer->height + y] = 0xFF00FF00;
        }
    }
#endif

    // We're done, just hang...
    KmHalt();
}
