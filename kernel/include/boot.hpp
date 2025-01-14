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
};

struct MemoryMapEntry {
    MemoryMapEntryType type;
    km::MemoryRange range;
};

static constexpr size_t kMaxMemoryMapEntries = 256;
using KernelMemoryMap = stdx::StaticVector<MemoryMapEntry, kMaxMemoryMapEntries>;

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

    void *address;

    km::MemoryRange edid;

    size_t size() const {
        return pitch * height;
    }
};

struct KernelLaunch {
    km::PhysicalAddress kernelPhysicalBase;
    const void *kernelVirtualBase;

    uintptr_t hhdmOffset;

    km::PhysicalAddress rsdpAddress;

    stdx::StaticVector<KernelFrameBuffer, 4> framebuffers;

    KernelMemoryMap memoryMap;

    km::MemoryRange stack;

    km::PhysicalAddress smbios32Address;
    km::PhysicalAddress smbios64Address;
};

extern "C" [[noreturn]] void KmLaunch(KernelLaunch launch);

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
            return "ACPI Reclaimable";
        case MemoryMapEntryType::eAcpiNvs:
            return "ACPI NVS";
        case MemoryMapEntryType::eBadMemory:
            return "Bad Memory";
        case MemoryMapEntryType::eBootloaderReclaimable:
            return "Bootloader Reclaimable";
        case MemoryMapEntryType::eKernel:
            return "Kernel and Modules";
        case MemoryMapEntryType::eFrameBuffer:
            return "Framebuffer";
        default:
            return "Unknown";
        }
    }
};
