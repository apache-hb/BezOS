#pragma once

#include "gdt.h"

#include "util/format.hpp"
#include "util/util.hpp"

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
    };

    UTIL_BITFLAGS(Access);

    constexpr uint64_t BuildSegmentDescriptor(Flags flags, Access access, uint32_t limit) {
        return (uint64_t)(flags) << 52
             | (uint64_t)(access) << 40
             | (uint16_t)(limit & 0xFFFF)
             | (uint64_t)(limit & 0xF0000) << 32;
    }

    class GdtEntry {
        uint64_t mValue;

    public:
        constexpr GdtEntry(Flags flags, Access access, uint32_t limit)
            : mValue(BuildSegmentDescriptor(flags, access, limit))
        { }

        constexpr GdtEntry() : mValue(0) { }

        static constexpr GdtEntry null() {
            return GdtEntry(Flags::eNone, Access::eNone, 0);
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

void KmInitGdt(const x64::GdtEntry *gdt, uint64_t count, uint64_t codeSelector, uint64_t dataSelector);

template<>
struct km::StaticFormat<x64::GdtEntry> {
    using String = stdx::StaticString<256>;
    static String toString(x64::GdtEntry value);
};

/// @brief The GDT entries for the system.
/// @warning This alignas(16) is load bearing, some AMD laptop cpus will hang forever
///          if the GDT is not 16 byte aligned.
struct SystemGdt {
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

        eCount = GDT_COUNT,
    };

    x64::GdtEntry entries[GDT_COUNT];
};

static_assert(std::is_standard_layout_v<x64::GdtEntry>);
static_assert(std::is_standard_layout_v<SystemGdt>);

SystemGdt KmGetSystemGdt();
