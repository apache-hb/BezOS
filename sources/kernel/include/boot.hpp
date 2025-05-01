#pragma once

#include "memory/layout.hpp"
#include "std/vector.hpp"
#include "allocator/tlsf.hpp"

#include <cstddef>

template<typename T>
void leak(T&& it) {
    union LeakHelper {
        ~LeakHelper() { }
        std::remove_cvref_t<T> value;
    };

    LeakHelper helper { std::move(it) };
}

namespace boot {
    static constexpr size_t kPrebootMemory = sm::kilobytes(64).bytes();

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

        km::AddressMapping mapping() const {
            return { vaddr, paddr, size() };
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
            eKernelStack,
        };

        Type type;
        km::MemoryRange range;

        size_t size() const { return range.size(); }

        bool isUsable() const;
        bool isReclaimable() const;
        bool isAccessible() const;
    };

    struct BootInfoBuilder {
        km::AddressMapping prebootMemory;
        km::AddressMapping bootMemory;
        mem::TlsfAllocator allocator;
        stdx::Vector3<MemoryRegion> regions;
        stdx::Vector3<FrameBuffer> framebuffers;

        BootInfoBuilder(km::AddressMapping mapping, size_t bootSize)
            : prebootMemory(km::AddressMapping { mapping.vaddr, mapping.paddr, kPrebootMemory })
            , bootMemory(km::AddressMapping { (void*)((uintptr_t)mapping.vaddr + kPrebootMemory), mapping.paddr + kPrebootMemory, bootSize })
            , allocator((void*)prebootMemory.vaddr, prebootMemory.size)
            , regions(&allocator)
            , framebuffers(&allocator)
        {
            regions.add(MemoryRegion {
                .type = MemoryRegion::eBootloaderReclaimable,
                .range = mapping.physicalRange(),
            });

            regions.add(MemoryRegion {
                .type = MemoryRegion::eReserved,
                .range = { 0zu, km::kLowMemory },
            });
        }

        void addRegion(MemoryRegion region) {
            if (region.range.overlaps(prebootMemory.physicalRange())) {
                region.range = region.range.cut(prebootMemory.physicalRange());
            }

            if (region.range.isEmpty()) return;

            regions.add(region);
        }

        void addDisplay(FrameBuffer fb) {
            framebuffers.add(fb);

            addRegion(MemoryRegion {
                .type = MemoryRegion::eFrameBuffer,
                .range = fb.mapping().physicalRange(),
            });
        }
    };

    constexpr sm::Memory UsableMemory(std::span<const MemoryRegion> regions) {
        size_t result = 0;
        for (const MemoryRegion& entry : regions) {
            if (entry.isUsable()) {
                result += entry.size();
            }
        }
        return sm::bytes(result);
    }

    constexpr sm::Memory ReclaimableMemory(std::span<const MemoryRegion> regions) {
        size_t result = 0;
        for (const MemoryRegion& entry : regions) {
            if (entry.isReclaimable()) {
                result += entry.size();
            }
        }
        return sm::bytes(result);
    }

    constexpr km::PhysicalAddress MaxPhysicalAddress(std::span<const MemoryRegion> regions) {
        km::PhysicalAddress result = nullptr;
        for (const MemoryRegion& entry : regions) {
            if (entry.isAccessible()) {
                result = std::max(result, entry.range.back);
            }
        }
        return result;
    }

    struct LaunchInfo {
        km::PhysicalAddress kernelPhysicalBase;
        const void *kernelVirtualBase;

        uintptr_t hhdmOffset;

        km::PhysicalAddress rsdpAddress;

        std::span<FrameBuffer> framebuffers;
        std::span<MemoryRegion> memmap;

        km::AddressMapping stack;

        km::PhysicalAddress smbios32Address;
        km::PhysicalAddress smbios64Address;

        km::MemoryRangeEx initrd;

        /// @brief Memory available to the kernel for use as an early allocator.
        km::AddressMapping earlyMemory;
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
        case boot::MemoryRegion::eKernelStack:
            return "Kernel stack";
        default:
            return "Unknown";
        }
    }
};
