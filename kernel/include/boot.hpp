#pragma once

#include "memory/memory.hpp"

#include <cstddef>

namespace boot {
    struct FrameBuffer {
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

    struct MemoryRegion {
        enum Type {
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

        Type type;
        km::MemoryRange range;

        size_t size() const { return range.size(); }

        bool isUsable() const;
        bool isReclaimable() const;
        bool isAccessible() const;
    };

    struct MemoryMap {
        std::span<MemoryRegion> regions;

        sm::Memory usableMemory() const;
        sm::Memory reclaimableMemory() const;

        km::PhysicalAddress maxPhysicalAddress() const;
    };

    struct LaunchInfo {
        km::PhysicalAddress kernelPhysicalBase;
        const void *kernelVirtualBase;

        uintptr_t hhdmOffset;

        km::PhysicalAddress rsdpAddress;

        std::span<FrameBuffer> framebuffers;
        MemoryMap memmap;

        km::AddressMapping stack;

        km::PhysicalAddress smbios32Address;
        km::PhysicalAddress smbios64Address;
    };
}

[[noreturn]]
void LaunchKernel(boot::LaunchInfo launch);

template<>
struct km::Format<boot::MemoryRegion::Type> {
    static constexpr size_t kStringSize = 16;
    static constexpr stdx::StringView toString(char *, boot::MemoryRegion::Type value) {
        switch (value) {
        case boot::MemoryRegion::eUsable:
            return "Usable";
        case boot::MemoryRegion::eReserved:
            return "Reserved";
        case boot::MemoryRegion::eAcpiReclaimable:
            return "ACPI reclaimable";
        case boot::MemoryRegion::eAcpiNvs:
            return "ACPI NVS";
        case boot::MemoryRegion::eBadMemory:
            return "Bad memory";
        case boot::MemoryRegion::eBootloaderReclaimable:
            return "Bootloader reclaimable";
        case boot::MemoryRegion::eKernel:
            return "Kernel and modules";
        case boot::MemoryRegion::eFrameBuffer:
            return "Framebuffer";
        case boot::MemoryRegion::eKernelRuntimeData:
            return "Kernel runtime data";
        default:
            return "Unknown";
        }
    }
};
