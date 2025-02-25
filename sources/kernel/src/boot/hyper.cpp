#include "boot.hpp"

#include <ultra_protocol.h>

template<typename T>
static const T *GetAttribute(const ultra_boot_context *context, uint32_t type) {
    uint32_t i = 0;
    const ultra_attribute_header *header = &context->attributes[0];

    while (i < context->attribute_count) {
        if (header->type == type) {
            return reinterpret_cast<const T*>(header);
        }

        header = ULTRA_NEXT_ATTRIBUTE(header);
        i += 1;
    }

    return nullptr;
}

static const ultra_module_info_attribute *GetModule(const ultra_boot_context *context, stdx::StringView name) {
    uint32_t i = 0;
    const ultra_attribute_header *header = &context->attributes[0];

    while (i < context->attribute_count) {
        if (header->type == ULTRA_ATTRIBUTE_MODULE_INFO) {
            const ultra_module_info_attribute *info = reinterpret_cast<const ultra_module_info_attribute*>(header);
            if (strncmp(info->name, name.data(), name.count()) == 0) {
                return info;
            }
        }

        header = ULTRA_NEXT_ATTRIBUTE(header);
        i += 1;
    }

    return nullptr;
}

static km::AddressMapping GetBootMemory(const ultra_boot_context *context) {
    auto bootstrap = GetModule(context, "allocator-bootstrap");
    void *address = (void*)bootstrap->address;

    return { address, (uintptr_t)address, bootstrap->size };
}

static boot::MemoryRegion::Type BootGetEntryType(ultra_memory_map_entry entry) {
    switch (entry.type) {
    case ULTRA_MEMORY_TYPE_FREE:
        return boot::MemoryRegion::eUsable;
    case ULTRA_MEMORY_TYPE_RESERVED:
        return boot::MemoryRegion::eReserved;
    case ULTRA_MEMORY_TYPE_RECLAIMABLE:
        return boot::MemoryRegion::eAcpiReclaimable;
    case ULTRA_MEMORY_TYPE_NVS:
        return boot::MemoryRegion::eAcpiNvs;
    case ULTRA_MEMORY_TYPE_LOADER_RECLAIMABLE:
        return boot::MemoryRegion::eBootloaderReclaimable;
    case ULTRA_MEMORY_TYPE_MODULE:
    case ULTRA_MEMORY_TYPE_KERNEL_BINARY:
        return boot::MemoryRegion::eKernel;
    case ULTRA_MEMORY_TYPE_KERNEL_STACK:
        return boot::MemoryRegion::eKernelStack;

    case ULTRA_MEMORY_TYPE_INVALID:
    default:
        return boot::MemoryRegion::eBadMemory;
    }
}

static void BootGetMemoryMap(const ultra_memory_map_attribute *memmap, boot::BootInfoBuilder& builder) {
    size_t size = ULTRA_MEMORY_MAP_ENTRY_COUNT(memmap->header);

    for (uint64_t i = 0; i < size; i++) {
        ultra_memory_map_entry entry = memmap->entries[i];

        boot::MemoryRegion::Type type = BootGetEntryType(entry);
        km::MemoryRange range = { entry.physical_address, entry.physical_address + entry.size };
        boot::MemoryRegion region = { type, range };

        builder.addRegion(region);
    }
}

static void BootGetFrameBuffers(uintptr_t hhdmOffset, const ultra_framebuffer_attribute *framebuffer, boot::BootInfoBuilder& builder) {
    ultra_framebuffer fb = framebuffer->fb;

    boot::FrameBuffer result {
        .width = fb.width,
        .height = fb.height,
        .pitch = fb.pitch,
        .bpp = fb.bpp,
        .paddr = fb.physical_address,
        .vaddr = (void*)(fb.physical_address + hhdmOffset),
    };

    switch (fb.format) {
    case ULTRA_FB_FORMAT_BGR888:
        result.redMaskSize = 8;
        result.redMaskShift = 16;
        result.greenMaskSize = 8;
        result.greenMaskShift = 8;
        result.blueMaskSize = 8;
        result.blueMaskShift = 0;
        break;
    case ULTRA_FB_FORMAT_RGB888:
        result.redMaskSize = 8;
        result.redMaskShift = 0;
        result.greenMaskSize = 8;
        result.greenMaskShift = 8;
        result.blueMaskSize = 8;
        result.blueMaskShift = 16;
        break;
    case ULTRA_FB_FORMAT_RGBX8888:
        result.redMaskSize = 8;
        result.redMaskShift = 0;
        result.greenMaskSize = 8;
        result.greenMaskShift = 8;
        result.blueMaskSize = 8;
        result.blueMaskShift = 16;
        break;
    case ULTRA_FB_FORMAT_XRGB8888:
        result.redMaskSize = 8;
        result.redMaskShift = 16;
        result.greenMaskSize = 8;
        result.greenMaskShift = 8;
        result.blueMaskSize = 8;
        result.blueMaskShift = 0;
        break;
    }

    builder.addDisplay(result);
}

static km::AddressMapping BootGetStack(const ultra_memory_map_attribute *memmap, uintptr_t hhdmOffset) {
    km::AddressMapping stack = { };

    for (uint64_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memmap->header); i++) {
        ultra_memory_map_entry entry = memmap->entries[i];

        if (entry.type == ULTRA_MEMORY_TYPE_KERNEL_STACK) {
            stack = { (void*)(entry.physical_address + hhdmOffset), km::PhysicalAddress { entry.physical_address }, entry.size };
            break;
        }
    }

    return stack;
}

static km::MemoryRange BootGetInitArchive(const ultra_boot_context *context) {
    auto mod = GetModule(context, "initrd");
    if (mod == nullptr) {
        return { };
    }

    return { mod->address, mod->address + mod->size };
}

extern "C" int HyperMain(ultra_boot_context *context, uint32_t) {
    auto framebuffer = GetAttribute<ultra_framebuffer_attribute>(context, ULTRA_ATTRIBUTE_FRAMEBUFFER_INFO);
    auto platformInfo = GetAttribute<ultra_platform_info_attribute>(context, ULTRA_ATTRIBUTE_PLATFORM_INFO);
    auto kernelInfo = GetAttribute<ultra_kernel_info_attribute>(context, ULTRA_ATTRIBUTE_KERNEL_INFO);
    auto memmap = GetAttribute<ultra_memory_map_attribute>(context, ULTRA_ATTRIBUTE_MEMORY_MAP);

    uintptr_t hhdmOffset = platformInfo->higher_half_base;
    km::AddressMapping bootMemory = GetBootMemory(context);
    boot::BootInfoBuilder builder { bootMemory, bootMemory.size - boot::kPrebootMemory };

    BootGetMemoryMap(memmap, builder);
    BootGetFrameBuffers(hhdmOffset, framebuffer, builder);

    km::AddressMapping stack = BootGetStack(memmap, hhdmOffset);
    km::MemoryRange initrd = BootGetInitArchive(context);

    boot::LaunchInfo info = {
        .kernelPhysicalBase = kernelInfo->physical_base,
        .kernelVirtualBase = (void*)kernelInfo->virtual_base,
        .hhdmOffset = hhdmOffset,
        .rsdpAddress = platformInfo->acpi_rsdp_address,
        .framebuffers = builder.framebuffers,
        .memmap = { builder.regions },
        .stack = stack,
        .smbios32Address = platformInfo->smbios_address,
        .smbios64Address = platformInfo->smbios_address,
        .initrd = initrd,
        .earlyMemory = builder.bootMemory,
    };

    leak(std::move(builder));

    LaunchKernel(info);
}
