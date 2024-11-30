// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limine.h>

// #include <x86intrin.h>

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

static inline void __DEFAULT_FN_ATTRS __halt(void) {
    asm volatile("hlt");
}

static inline void __DEFAULT_FN_ATTRS __lgdt(struct GDTR gdtr) {
    asm volatile("lgdt %0" :: "m"(gdtr));
}

static inline unsigned char __DEFAULT_FN_ATTRS __inbyte(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline unsigned short __DEFAULT_FN_ATTRS __inword(unsigned short port) {
    unsigned short ret;
    asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline unsigned long __DEFAULT_FN_ATTRS __indword(unsigned short port) {
    unsigned long ret;
    asm volatile("inl %w1, %k0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void __DEFAULT_FN_ATTRS __outbyte(unsigned short port, unsigned char data) {
    asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

static inline void __DEFAULT_FN_ATTRS __outword(unsigned short port, unsigned short data) {
    asm volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));
}

static inline void __DEFAULT_FN_ATTRS __outdword(unsigned short port, unsigned long data) {
    asm volatile("outl %k0, %w1" : : "a"(data), "Nd"(port));
}

static inline void __DEFAULT_FN_ATTRS __nop(void) {
    asm volatile("nop");
}

static inline void __DEFAULT_FN_ATTRS __cli(void) {
    asm volatile("cli");
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

static void hcf(void) {
    for (;;) {
        __halt();
    }
}

static void e9print(const char *text) {
    while (*text) {
        __outbyte(0xE9, *text++);
    }
}

static void e9print_hex(uint64_t val) {
    e9print("0x");
    const char *hex = "0123456789ABCDEF";
    for (int i = 60; i >= 0; i -= 4) {
        __outbyte(0xE9, hex[(val >> i) & 0xF]);
    }
}

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

enum DescriptorFlags {
    eDescriptorFlag_Long = (1 << 1),
    eDescriptorFlag_Size = (1 << 2),
    eDescriptorFlag_Granularity = (1 << 3),
};

enum SegmentAccessFlags : uint8_t {
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

static uint64_t BuildSegmentDescriptor(enum DescriptorFlags flags, enum SegmentAccessFlags access, uint16_t limit) {
    return (((uint64_t)(flags) << 52) | ((uint64_t)(access) << 40)) | (0xFULL << 48) | (uint16_t)limit;
}

enum GdtEntry {
    eGdtEntry_Null = 0,
    eGdtEntry_Ring0Code = 1,
    eGdtEntry_Ring0Data = 2,

    eGdtEntry_Count
};

static uint64_t gGdtEntries[eGdtEntry_Count];

static void BuildGdt(void) {
    gGdtEntries[eGdtEntry_Null] = 0;
    gGdtEntries[eGdtEntry_Ring0Code] = BuildSegmentDescriptor(
        eDescriptorFlag_Long | eDescriptorFlag_Granularity,
        eSegmentAccess_Executable | eSegmentAccess_CodeOrDataSegment | eSegmentAccess_ReadWrite | eSegmentAccess_Ring0 | eSegmentAccess_Present | eSegmentAccess_Accessed,
        0xFFFF
    );
    gGdtEntries[eGdtEntry_Ring0Data] = BuildSegmentDescriptor(
        eDescriptorFlag_Long | eDescriptorFlag_Granularity,
        eSegmentAccess_CodeOrDataSegment | eSegmentAccess_Ring0 | eSegmentAccess_ReadWrite | eSegmentAccess_Present | eSegmentAccess_Accessed,
        0xFFFF
    );

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

#if 0
static void FlushGdt(unsigned short codeSegment, unsigned short dataSegment) {
    codeSegment *= sizeof(uint64_t);
    dataSegment *= sizeof(uint64_t);
    asm volatile (
        "push %[data]\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov %[data], %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ret\n"
        : /* no outputs */
        : [code] "r"(codeSegment), [data] "r"(dataSegment) /* inputs */
        : "memory" /* clobbers */
    );
}
#endif


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

    if (type < sizeof(kMemoryMapTypeNames) / sizeof(kMemoryMapTypeNames[0])) {
        return kMemoryMapTypeNames[type];
    }
    return "Unknown";
}

void kmain(void) {
    __cli();

    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    struct limine_memmap_response *memmap = memmap_request.response;
    if (memmap == NULL || memmap->entry_count < 1) {
        e9print("No memory map!\n");
        hcf();
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry == NULL) {
            e9print("Memory map entry is NULL!\n");
            hcf();
        }
        e9print("Memory map entry: ");
        e9print_hex(entry->base);
        e9print(" ");
        e9print_hex(entry->length);
        e9print(" ");
        e9print(GetMemoryMapTypeName(entry->type));
        e9print("\n");
    }

    e9print("Hello World.\n");
    BuildGdt();
    e9print("GDT built.\n");

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
    hcf();
}
