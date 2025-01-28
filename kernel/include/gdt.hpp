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
        uint8_t reserved3[2];
        uint16_t iopbOffset;
    };

    static_assert(sizeof(TaskStateSegment) == 0x68);

    struct TssEntry {
        uint16_t limit0;
        uint16_t address0;
        uint8_t address1;
        uint8_t access;
        uint8_t flags;
        uint8_t address2;
        uint32_t address3;
        uint32_t reserved;
    };

    static_assert(sizeof(TssEntry) == 0x10);

    static constexpr TssEntry NewTssEntry(const TaskStateSegment *tss, uint8_t dpl) {
        uintptr_t address = (uintptr_t)tss;
        return TssEntry {
            .limit0 = sizeof(TaskStateSegment) - 1,
            .address0 = uint16_t(address & 0xFFFF),
            .address1 = uint8_t((address >> 16) & 0xFF),
            .access = uint8_t(0b10001001 | ((dpl & 0b11) << 5)),
            .flags = 0,
            .address2 = uint8_t((address >> 24) & 0xFF),
            .address3 = uint32_t(address >> 32),
            .reserved = 0,
        };
    }

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

namespace km {
    void InitGdt(const void *gdt, size_t size, size_t codeSelector, size_t dataSelector);
    void InitGdt(std::span<const x64::GdtEntry> gdt, size_t codeSelector, size_t dataSelector);

    /// @brief The GDT entries for the system.
    /// @warning This alignas(0x10) is load bearing, gdt entries not aligned to 16 bytes can
    ///          cause faults on some processors.
    struct alignas(0x10) SystemGdt {
        enum : int {
            eNull = GDT_NULL,
            eRealModeCode = GDT_16BIT_CODE,
            eRealModeData = GDT_16BIT_DATA,
            eProtectedModeCode = GDT_32BIT_CODE,
            eProtectedModeData = GDT_32BIT_DATA,
            eLongModeCode = GDT_64BIT_CODE,
            eLongModeData = GDT_64BIT_DATA,
            eLongModeUserData = GDT_64BIT_USER_DATA,
            eLongModeUserCode = GDT_64BIT_USER_CODE,
            eTaskState0 = GDT_TSS0,
            eTaskState1 = GDT_TSS1,

            eCount = GDT_TSS_COUNT,
        };

        x64::GdtEntry entries[eCount];
    };

    static_assert(std::is_standard_layout_v<x64::GdtEntry>);
    static_assert(std::is_standard_layout_v<SystemGdt>);

    SystemGdt GetBootGdt();

    SystemGdt GetSystemGdt(const x64::TaskStateSegment *tss);
}

template<>
struct km::Format<x64::GdtEntry> {
    using String = stdx::StaticString<256>;
    static String toString(x64::GdtEntry value);
};
