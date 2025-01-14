#pragma once

#include "gdt.h"

#include "util/format.hpp"
#include "util/util.hpp"

#include <span>
#include <stdint.h>
#include <type_traits>

namespace x64 {
    enum class Flags {
        eNone = 0,
        eLong = (1 << 1),
        eSize = (1 << 2),
        eGranularity = (1 << 3),

        eRealMode = eNone,
        eProtectedMode = eSize | eGranularity,
        eLongMode = eLong | eGranularity,
    };

    UTIL_BITFLAGS(Flags);

    enum class Access {
        eNone = 0,

        eAccessed = (1 << 0),
        eReadWrite = (1 << 1),
        eEscalateDirection = (1 << 2),
        eExecutable = (1 << 3),
        eCodeOrDataSegment = (1 << 4),

        eRing0 = (0 << 5),
        eRing1 = (1 << 5),
        eRing2 = (2 << 5),
        eRing3 = (3 << 5),

        ePresent = (1 << 7),

        eData = eCodeOrDataSegment | eReadWrite | ePresent | eAccessed,
        eCode = eCodeOrDataSegment | eExecutable | eReadWrite | ePresent | eAccessed,
        eTaskState = eAccessed | ePresent,
    };

    UTIL_BITFLAGS(Access);

    constexpr uint64_t BuildSegmentDescriptor(Flags flags, Access access, uint32_t limit, uint32_t base = 0) {
        return (uint64_t)(base & 0xFF000000) << 32
             | (uint64_t)(flags) << 52
             | (uint64_t)(access) << 40
             | (uint64_t)(limit & 0xF0000) << 32
             | (uint64_t)(base & 0xFFFFFF) << 16
             | (uint16_t)(limit & 0xFFFF);
    }

    struct [[gnu::packed]] TaskStateSegment {
        uint8_t reserved0[4];
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;
        uint8_t reserved1[8];
        uint64_t ist1;
        uint64_t ist2;
        uint64_t ist3;
        uint64_t ist4;
        uint64_t ist5;
        uint64_t ist6;
        uint64_t ist7;
        uint8_t reserved2[8];
        uint16_t iopbOffset;
        uint8_t reserved3[2];
    };

    static_assert(sizeof(TaskStateSegment) == 0x68);

    class GdtEntry {
        uint64_t mValue;

        constexpr GdtEntry(uint64_t value)
            : mValue(value)
        { }

    public:
        constexpr GdtEntry(Flags flags, Access access, uint32_t limit, uint32_t base = 0)
            : GdtEntry(BuildSegmentDescriptor(flags, access, limit, base))
        { }

        constexpr GdtEntry() : GdtEntry(0) { }

        static constexpr GdtEntry null() {
            return GdtEntry(Flags::eNone, Access::eNone, 0);
        }

        static constexpr GdtEntry tss0(const TaskStateSegment *tss) {
            uintptr_t address = (uintptr_t)tss;
            return sizeof(TaskStateSegment)
                 | (address & 0xFFFF) << 16
                 | (address & 0xFF0000) << 32
                 | (0b10001001ull) << 40
                 | (address & 0xFF000000) << 52;
        }

        static constexpr GdtEntry tss1(const TaskStateSegment *tss) {
            uintptr_t address = (uintptr_t)tss;
            return address >> 32;
        }

        uint64_t value() const { return mValue; }

        uint32_t limit() const {
            uint32_t limit = mValue & 0xFFFF;
            limit |= (mValue >> 32) & 0xF0000;
            return limit;
        }

        uint32_t base() const {
            uint32_t base = (mValue >> 16) & 0xFFFF;
            base |= (mValue >> 32) & 0xFF000000;
            return base;
        }

        Flags flags() const {
            return Flags(mValue >> 52);
        }

        Access access() const {
            return Access((mValue >> 40) & 0xFF);
        }
    };

    constexpr uint64_t kNullDescriptor = 0;
}

void KmInitGdt(std::span<const x64::GdtEntry> gdt, size_t codeSelector, size_t dataSelector);

template<>
struct km::Format<x64::GdtEntry> {
    using String = stdx::StaticString<256>;
    static String toString(x64::GdtEntry value);
};

/// @brief The GDT entries for the system.
/// @warning This alignas(8) is load bearing, unaligned gdt entries can cause poor performance.
struct alignas(8) SystemGdt {
    enum : int {
        eNull = GDT_NULL,
        eRealModeCode = GDT_16BIT_CODE,
        eRealModeData = GDT_16BIT_DATA,
        eProtectedModeCode = GDT_32BIT_CODE,
        eProtectedModeData = GDT_32BIT_DATA,
        eLongModeCode = GDT_64BIT_CODE,
        eLongModeData = GDT_64BIT_DATA,
        eLongModeUserCode = GDT_64BIT_USER_CODE,
        eLongModeUserData = GDT_64BIT_USER_DATA,
        eTaskState0 = GDT_TSS0,
        eTaskState1 = GDT_TSS1,

        eCount = GDT_COUNT,
    };

    x64::GdtEntry entries[GDT_COUNT];
};

static_assert(std::is_standard_layout_v<x64::GdtEntry>);
static_assert(std::is_standard_layout_v<SystemGdt>);

SystemGdt KmGetBootGdt();

SystemGdt KmGetSystemGdt(const x64::TaskStateSegment *tss);
