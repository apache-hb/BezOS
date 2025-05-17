#pragma once

#include "arch/isr.hpp"
#include "gdt.h"

#include "util/format.hpp"
#include "common/util/util.hpp"

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

    class [[gnu::packed]] TssEntry {
        uint16_t mLimit0;
        uint16_t mAddress0;
        uint8_t mAddress1;
        uint8_t mAccess;
        uint8_t mFlags;
        uint8_t mAddress2;
        uint32_t mAddress3;

        [[maybe_unused]]
        uint32_t mReserved;

        constexpr TssEntry(uintptr_t tssAddress, uint8_t dpl) noexcept
            : mLimit0(sizeof(TaskStateSegment) - 1)
            , mAddress0(uint16_t(tssAddress & 0xFFFF))
            , mAddress1(uint8_t((tssAddress >> 16) & 0xFF))
            , mAccess(uint8_t(0b10001001 | (dpl << 5)))
            , mFlags(0)
            , mAddress2(uint8_t((tssAddress >> 24) & 0xFF))
            , mAddress3(uint32_t(tssAddress >> 32))
            , mReserved(0)
        { }

    public:
        constexpr TssEntry() noexcept = default;

        constexpr TssEntry(const TaskStateSegment *tss, Privilege dpl) noexcept
            : TssEntry(std::bit_cast<uintptr_t>(tss), std::to_underlying(dpl))
        { }

        bool isPresent() const noexcept {
            return (mAccess & (1 << 7)) != 0;
        }

        constexpr Privilege dpl() const noexcept {
            return Privilege((mAccess >> 5) & 0b11);
        }

        constexpr uint64_t address() const noexcept {
            return (uint64_t(mAddress3) << 32) | (uint64_t(mAddress2) << 24) | (uint64_t(mAddress1) << 16) | mAddress0;
        }

        constexpr uint32_t limit() const noexcept {
            return mLimit0 | (uint32_t(mFlags & 0xF) << 16);
        }

        constexpr bool granularity() const noexcept {
            return (mFlags & (1 << 7));
        }
    };

    static_assert(sizeof(TssEntry) == 0x10);

    static constexpr TssEntry NewTssEntry(const TaskStateSegment *tss, uint8_t dpl) {
        return TssEntry(tss, Privilege(dpl));
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

        static constexpr GdtEntry null() noexcept {
            return GdtEntry(Flags::eNone, Access::eNone, 0);
        }

        constexpr uint64_t value() const noexcept { return mValue; }

        constexpr uint32_t limit() const noexcept {
            uint32_t limit = mValue & 0xFFFF;
            limit |= (mValue >> 32) & 0xF0000;
            return limit;
        }

        constexpr uint32_t base() const noexcept {
            uint32_t base = (mValue >> 16) & 0xFFFF;
            base |= (mValue >> 32) & 0xFF000000;
            return base;
        }

        constexpr Flags flags() const noexcept {
            return Flags(mValue >> 52);
        }

        constexpr Access access() const noexcept {
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

        void setTss(const x64::TaskStateSegment *tss) {
            x64::TssEntry entry = x64::NewTssEntry(tss, 0);

            memcpy(&entries[SystemGdt::eTaskState0], &entry, sizeof(entry));
        }
    };

    static_assert(std::is_standard_layout_v<x64::GdtEntry>);
    static_assert(std::is_standard_layout_v<SystemGdt>);

    [[gnu::const]]
    SystemGdt GetBootGdt() noexcept;

    SystemGdt GetSystemGdt(const x64::TaskStateSegment *tss);
}

template<>
struct km::Format<x64::GdtEntry> {
    using String = stdx::StaticString<256>;
    static String toString(x64::GdtEntry value);
};
