#include "boot.hpp"

#include <limine.h>

static constexpr size_t kStackSize = sm::kilobytes(16).bytes();
static constexpr size_t kBootMemory = sm::kilobytes(64).bytes();
static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

[[gnu::used, gnu::section(".limine_requests")]]
static volatile LIMINE_BASE_REVISION(3)

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_memmap_request gMemmoryMapRequest = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_kernel_address_request gExecutableAddressRequest = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_hhdm_request gDirectMapRequest = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_rsdp_request gAcpiTableRequest = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_framebuffer_request gFramebufferRequest = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_smbios_request gSmbiosRequest = {
    .id = LIMINE_SMBIOS_REQUEST,
    .revision = 0,
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_module_request gModuleRequest = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
};

[[gnu::used, gnu::section(".limine_requests_start")]]
static volatile LIMINE_REQUESTS_START_MARKER

[[gnu::used, gnu::section(".limine_requests_end")]]
static volatile LIMINE_REQUESTS_END_MARKER

struct BootAllocator {
    km::AddressMapping addr;

    size_t offset = 0;

    km::MemoryRange range() const {
        return addr.physicalRange();
    }

    void *malloc(size_t size, size_t align = alignof(std::max_align_t)) {
        if (offset + size > addr.size) {
            return nullptr;
        }

        size_t offsetAlign = (size_t)addr.vaddr % align;
        if (offsetAlign != 0) {
            offset += align - offsetAlign;
        }

        void *ptr = (void*)((size_t)addr.vaddr + offset);
        offset += size;

        return ptr;
    }
};

static boot::FrameBuffer BootGetDisplay(int index, uintptr_t hhdmOffset) {
    limine_framebuffer_response response = *gFramebufferRequest.response;
    limine_framebuffer framebuffer = *response.framebuffers[index];

    km::PhysicalAddress edidAddress = km::PhysicalAddress { (uintptr_t)framebuffer.edid };

    return boot::FrameBuffer {
        .width = framebuffer.width,
        .height = framebuffer.height,
        .pitch = framebuffer.pitch,
        .bpp = framebuffer.bpp,
        .redMaskSize = framebuffer.red_mask_size,
        .redMaskShift = framebuffer.red_mask_shift,
        .greenMaskSize = framebuffer.green_mask_size,
        .greenMaskShift = framebuffer.green_mask_shift,
        .blueMaskSize = framebuffer.blue_mask_size,
        .blueMaskShift = framebuffer.blue_mask_shift,
        .paddr = km::PhysicalAddress { (uintptr_t)framebuffer.address - hhdmOffset },
        .vaddr = framebuffer.address,
        .edid = { edidAddress, edidAddress + framebuffer.edid_size }
    };
}

static std::span<boot::FrameBuffer> BootGetFrameBuffers(uintptr_t hhdmOffset, BootAllocator& alloc) {
    limine_framebuffer_response response = *gFramebufferRequest.response;
    boot::FrameBuffer *fbs = (boot::FrameBuffer*)alloc.malloc(sizeof(boot::FrameBuffer) * response.framebuffer_count, alignof(boot::FrameBuffer));

    for (uint64_t i = 0; i < response.framebuffer_count; i++) {
        fbs[i] = BootGetDisplay(i, hhdmOffset);
    }

    return { fbs, response.framebuffer_count };
}

static boot::MemoryRegion::Type BootGetEntryType(limine_memmap_entry entry) {
    switch (entry.type) {
    case LIMINE_MEMMAP_USABLE:
        return boot::MemoryRegion::eUsable;
    case LIMINE_MEMMAP_RESERVED:
        return boot::MemoryRegion::eReserved;
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return boot::MemoryRegion::eAcpiReclaimable;
    case LIMINE_MEMMAP_ACPI_NVS:
        return boot::MemoryRegion::eAcpiNvs;
    case LIMINE_MEMMAP_BAD_MEMORY:
        return boot::MemoryRegion::eBadMemory;
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return boot::MemoryRegion::eBootloaderReclaimable;
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:
        return boot::MemoryRegion::eKernel;
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return boot::MemoryRegion::eFrameBuffer;
    default:
        return boot::MemoryRegion::eBadMemory;
    }
}

static std::span<boot::MemoryRegion> BootGetMemoryMap(BootAllocator& alloc) {
    limine_memmap_response response = *gMemmoryMapRequest.response;
    boot::MemoryRegion *memmap = (boot::MemoryRegion*)alloc.malloc(sizeof(boot::MemoryRegion) * response.entry_count + 1, alignof(boot::MemoryRegion));

    for (uint64_t i = 0; i < response.entry_count; i++) {
        limine_memmap_entry entry = *response.entries[i];

        boot::MemoryRegion::Type type = BootGetEntryType(entry);

        km::MemoryRange range = { entry.base, entry.base + entry.length };
        if (range.overlaps(alloc.range())) {
            range = range.cut(alloc.range());
        }

        boot::MemoryRegion item = { type, range };

        memmap[i] = item;
    }

    // Add the boot memory to the memory map
    memmap[response.entry_count] = { boot::MemoryRegion::eBootloaderReclaimable, alloc.range() };

    return { memmap, response.entry_count + 1 };
}

static BootAllocator MakeBootAllocator(size_t size, const limine_memmap_response& memmap, uintptr_t hhdmOffset) {
    // Find a free memory range to use as a buffer
    for (uint64_t i = 0; i < memmap.entry_count; i++) {
        limine_memmap_entry entry = *memmap.entries[i];

        if (entry.type != LIMINE_MEMMAP_USABLE)
            continue;

        if (entry.base > kLowMemory)
            continue;

        if (entry.length < size)
            continue;

        km::AddressMapping addr = { (void*)(entry.base + hhdmOffset), km::PhysicalAddress { entry.base }, size };

        return BootAllocator { addr };
    }

    return BootAllocator { };
}

static km::MemoryRange GetInitDiskImage(const limine_module_response *modules, uintptr_t hhdmOffset) {
    if (modules == nullptr) {
        return { };
    }

    if (modules->module_count == 0) {
        return { };
    }

    // For now we assume the first module is the initrd

    limine_file file = *modules->modules[0];
    uintptr_t front = (uintptr_t)file.address - hhdmOffset;
    uintptr_t back = front + file.size;
    return { front, back };
}

extern "C" void LimineMain(void) {
    // offset the stack pointer as limine pushes qword 0 to
    // the stack before jumping to the kernel. and builtin_frame_address
    // returns the address where call would store the return address.
    const char *base = (char*)__builtin_frame_address(0) + (sizeof(void*) * 2);

    limine_kernel_address_response kernelAddress = *gExecutableAddressRequest.response;
    limine_memmap_response memory = *gMemmoryMapRequest.response;
    limine_hhdm_response hhdm = *gDirectMapRequest.response;
    limine_rsdp_response rsdp = *gAcpiTableRequest.response;
    limine_smbios_response smbios = *gSmbiosRequest.response;
    const limine_module_response *modules = gModuleRequest.response;
    uintptr_t stack = (uintptr_t)base - hhdm.offset;

    BootAllocator alloc = MakeBootAllocator(kBootMemory, memory, hhdm.offset);
    std::span<boot::FrameBuffer> framebuffers = BootGetFrameBuffers(hhdm.offset, alloc);
    std::span<boot::MemoryRegion> memmap = BootGetMemoryMap(alloc);
    km::MemoryRange initrd = GetInitDiskImage(modules, hhdm.offset);

    boot::LaunchInfo info = {
        .kernelPhysicalBase = kernelAddress.physical_base,
        .kernelVirtualBase = (void*)kernelAddress.virtual_base,
        .hhdmOffset = hhdm.offset,
        .rsdpAddress = (uintptr_t)rsdp.address,
        .framebuffers = framebuffers,
        .memmap = { memmap },
        .stack = { base - kStackSize, stack - kStackSize, kStackSize },
        .smbios32Address = (uintptr_t)smbios.entry_32,
        .smbios64Address = (uintptr_t)smbios.entry_64,
        .initrd = initrd,
    };

    LaunchKernel(info);
}
