#include "kernel.hpp"

#include <limine.h>

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

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_smbios_request gSmbiosRequest = {
    .id = LIMINE_SMBIOS_REQUEST,
    .revision = 0
};

[[gnu::used, gnu::section(".limine_requests_start")]]
static volatile LIMINE_REQUESTS_START_MARKER

[[gnu::used, gnu::section(".limine_requests_end")]]
static volatile LIMINE_REQUESTS_END_MARKER

static KernelFrameBuffer BootGetDisplay(uintptr_t hhdmOffset, int index) {
    limine_framebuffer_response response = *gFramebufferRequest.response;
    limine_framebuffer framebuffer = *response.framebuffers[index];

    km::PhysicalAddress edidAddress = km::PhysicalAddress { (uintptr_t)framebuffer.edid };

    return KernelFrameBuffer {
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
        .address = (uintptr_t)framebuffer.address - hhdmOffset,
        .edid = { edidAddress, edidAddress + framebuffer.edid_size }
    };
}

static stdx::StaticVector<KernelFrameBuffer, 4> BootGetAllDisplays(uintptr_t hhdmOffset) {
    limine_framebuffer_response response = *gFramebufferRequest.response;
    stdx::StaticVector<KernelFrameBuffer, 4> result;

    for (uint64_t i = 0; i < std::min<uint64_t>(result.capacity(), response.framebuffer_count); i++) {
        result.add(BootGetDisplay(hhdmOffset, i));
    }

    return result;
}

static MemoryMapEntryType BootGetEntryType(limine_memmap_entry entry) {
    switch (entry.type) {
    case LIMINE_MEMMAP_USABLE:
        return MemoryMapEntryType::eUsable;
    case LIMINE_MEMMAP_RESERVED:
        return MemoryMapEntryType::eReserved;
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return MemoryMapEntryType::eAcpiReclaimable;
    case LIMINE_MEMMAP_ACPI_NVS:
        return MemoryMapEntryType::eAcpiNvs;
    case LIMINE_MEMMAP_BAD_MEMORY:
        return MemoryMapEntryType::eBadMemory;
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return MemoryMapEntryType::eBootloaderReclaimable;
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:
        return MemoryMapEntryType::eKernel;
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return MemoryMapEntryType::eFrameBuffer;
    default:
        return MemoryMapEntryType::eBadMemory;
    }
}

static KernelMemoryMap BootGetMemoryMap(void) {
    limine_memmap_response response = *gMemmoryMapRequest.response;

    KernelMemoryMap result;
    for (uint64_t i = 0; i < response.entry_count; i++) {
        limine_memmap_entry entry = *response.entries[i];

        MemoryMapEntryType type = BootGetEntryType(entry);

        MemoryMapEntry item = { type, { entry.base, entry.base + entry.length } };

        result.add(item);
    }

    return result;
}

extern "C" void kmain(void) {
    // KM_CHECK(LIMINE_BASE_REVISION_SUPPORTED, "Unsupported limine base revision.");

    // offset the stack pointer as limine pushes qword 0 to
    // the stack before jumping to the kernel. and builtin_frame_address
    // returns the address where call would store the return address.
    const char *base = (char*)__builtin_frame_address(0) + (sizeof(void*) * 2);

    limine_kernel_address_response kernelAddress = *gExecutableAddressRequest.response;
    limine_hhdm_response hhdm = *gDirectMapRequest.response;
    limine_rsdp_response rsdp = *gAcpiTableRequest.response;
    limine_smbios_response smbios = *gSmbiosRequest.response;
    uintptr_t stack = (uintptr_t)base - hhdm.offset;

    KernelLaunch launch = {
        .kernelPhysicalBase = kernelAddress.physical_base,
        .kernelVirtualBase = (void*)kernelAddress.virtual_base,
        .hhdmOffset = hhdm.offset,
        .rsdpAddress = (uintptr_t)rsdp.address,
        .framebuffers = BootGetAllDisplays(hhdm.offset),
        .memoryMap = BootGetMemoryMap(),
        .stack = { stack - 0x10000, stack },
        .smbios32Address = (uintptr_t)smbios.entry_32,
        .smbios64Address = (uintptr_t)smbios.entry_64,
    };

    KmLaunch(launch);
}
