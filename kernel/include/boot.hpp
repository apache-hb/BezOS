#pragma once

#include "memory/memory.hpp"
#include "std/static_vector.hpp"

#include <cstddef>

enum class MemoryMapEntryType {
    eUsable,
    eReserved,
    eAcpiReclaimable,
    eAcpiNvs,
    eBadMemory,
    eBootloaderReclaimable,
    eKernel,
    eFrameBuffer,
    eKernelRuntimeData,
};

struct MemoryMapEntry {
    MemoryMapEntryType type;
    km::MemoryRange range;

    size_t size() const { return range.size(); }
    bool isUsable() const {
        switch (type) {
        case MemoryMapEntryType::eUsable:
        case MemoryMapEntryType::eAcpiReclaimable:
        case MemoryMapEntryType::eBootloaderReclaimable:
            return true;

        default:
            return false;
        }
    }
};

struct KernelFrameBuffer {
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;

    uint8_t redMaskSize;
    uint8_t redMaskShift;

    uint8_t greenMaskSize;
    uint8_t greenMaskShift;

    uint8_t blueMaskSize;
    uint8_t blueMaskShift;

    km::PhysicalAddress paddr;
    void *vaddr;

    km::MemoryRange edid;

    size_t size() const {
        return pitch * height;
    }
};

namespace boot {
    using FrameBuffer = KernelFrameBuffer;
    using MemoryRegion = MemoryMapEntry;

    struct LaunchInfo {
        km::PhysicalAddress kernelPhysicalBase;
        const void *kernelVirtualBase;

        uintptr_t hhdmOffset;

        km::PhysicalAddress rsdpAddress;

        std::span<boot::FrameBuffer> framebuffers;
        std::span<boot::MemoryRegion> memmap;

        km::AddressMapping stack;

        km::PhysicalAddress smbios32Address;
        km::PhysicalAddress smbios64Address;
    };
}

[[noreturn]]
void KmLaunchEx(boot::LaunchInfo launch);

template<>
struct km::Format<MemoryMapEntryType> {
    static constexpr size_t kStringSize = 16;
    static constexpr stdx::StringView toString(char *, MemoryMapEntryType value) {
        switch (value) {
        case MemoryMapEntryType::eUsable:
            return "Usable";
        case MemoryMapEntryType::eReserved:
            return "Reserved";
        case MemoryMapEntryType::eAcpiReclaimable:
            return "ACPI reclaimable";
        case MemoryMapEntryType::eAcpiNvs:
            return "ACPI NVS";
        case MemoryMapEntryType::eBadMemory:
            return "Bad memory";
        case MemoryMapEntryType::eBootloaderReclaimable:
            return "Bootloader reclaimable";
        case MemoryMapEntryType::eKernel:
            return "Kernel and modules";
        case MemoryMapEntryType::eFrameBuffer:
            return "Framebuffer";
        case MemoryMapEntryType::eKernelRuntimeData:
            return "Kernel runtime data";
        default:
            return "Unknown";
        }
    }
};
