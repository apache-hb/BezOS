#pragma once

#include <stddef.h>

#include "memory/memory.hpp"

#include "std/static_vector.hpp"
#include "std/string_view.hpp"

#include "util/format.hpp"

void KmBeginWrite();
void KmEndWrite();

void KmDebugWrite(stdx::StringView value = "\n");

template<km::IsStaticFormat T>
void KmDebugWrite(const T& value) {
    char buffer[km::StaticFormat<T>::kStringSize];
    KmDebugWrite(km::StaticFormat<T>::toString(buffer, value));
}

template<km::IsStaticFormatEx T>
void KmDebugWrite(const T& value) {
    auto result = km::format(value);
    KmDebugWrite(result);
}

template<km::IsStaticFormat T>
void KmDebugWrite(km::FormatOf<T> value) {
    char buffer[km::StaticFormat<T>::kStringSize];
    stdx::StringView result = km::StaticFormat<T>::toString(buffer, value.value);
    if (value.specifier.width <= result.count()) {
        KmDebugWrite(result);
        return;
    }

    if (value.specifier.align == km::Align::eRight) {
        KmDebugWrite(result);
        for (int i = result.count(); i < value.specifier.width; i++) {
            KmDebugWrite(value.specifier.fill);
        }
    } else {
        for (int i = result.count(); i < value.specifier.width; i++) {
            KmDebugWrite(value.specifier.fill);
        }
        KmDebugWrite(result);
    }
}

template<typename... T>
void KmDebugMessage(T&&... args) {
    KmBeginWrite();
    (KmDebugWrite(args), ...);
    KmEndWrite();
}

void KmSetupApGdt(void);

extern "C" [[noreturn]] void KmHalt(void);

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

    km::PhysicalAddress address;

    km::MemoryRange edid;

    size_t size() const {
        return pitch * height * bpp / 8;
    }
};

struct KernelLaunch {
    km::PhysicalAddress kernelPhysicalBase;
    km::VirtualAddress kernelVirtualBase;

    uintptr_t hhdmOffset;

    km::PhysicalAddress rsdpAddress;

    stdx::StaticVector<KernelFrameBuffer, 4> framebuffers;

    KernelMemoryMap memoryMap;

    km::MemoryRange stack;

    km::PhysicalAddress smbios32Address;
    km::PhysicalAddress smbios64Address;
};

extern "C" [[noreturn]] void KmLaunch(KernelLaunch launch);

[[noreturn]]
void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line);

#define KM_PANIC(msg) KmBugCheck(msg, __FILE__, __LINE__)
#define KM_CHECK(expr, msg) do { if (!(expr)) { KmBugCheck(msg, __FILE__, __LINE__); } } while (0)

template<>
struct km::StaticFormat<MemoryMapEntryType> {
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
