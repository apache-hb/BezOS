#pragma once

#include "arch/intrin.hpp"
#include "arch/paging.hpp"

#include "memory/memory.hpp"
#include "util/format.hpp"

namespace km {
    /// @brief Page Attribute Table (PAT) and Memory Type Range Registers (MTRRs) choices.
    /// @see AMD64 Architecture Programmer's Manual Volume 2: System Programming Table 7-9.
    enum class MemoryType {
        /// @brief No caching, write combining, or speculative reads are allowed.
        eUncached            = 0x00,

        /// @brief Only write combining is allowed,
        eWriteCombine       = 0x01,

        /// @brief Only speculative reads are allowed.
        eWriteThrough        = 0x04,

        /// @brief All writes update main memory.
        eWriteProtect        = 0x05,

        /// @brief All reads and writes can be cached.
        eWriteBack           = 0x06,

        /// @brief Same as @a eUncached, but can be overridden by the MTRRs.
        /// @note Not valid for @a MemoryTypeRanges.
        eUncachedOverridable = 0x07,
    };

    struct PageMemoryTypeLayout {
        /// @brief Deferred to the MTRRs. UC-
        uint8_t deferred;
        uint8_t uncached;
        uint8_t writeCombined;
        uint8_t writeThrough;
        uint8_t writeProtect;
        uint8_t writeBack;
    };

    class PageBuilder {
        uintptr_t mAddressMask;
        uint64_t mHigherHalfOffset;
        PageMemoryTypeLayout mLayout;

        uint8_t getMemoryTypeIndex(MemoryType type) const {
            switch (type) {
            case MemoryType::eUncached:
                return mLayout.uncached;
            case MemoryType::eWriteCombine:
                return mLayout.writeCombined;
            case MemoryType::eWriteThrough:
                return mLayout.writeThrough;
            case MemoryType::eWriteProtect:
                return mLayout.writeProtect;
            case MemoryType::eWriteBack:
                return mLayout.writeBack;
            default:
                return mLayout.deferred;
            }
        }

    public:
        PageBuilder(uintptr_t bits, uint64_t hhdmOffset, PageMemoryTypeLayout layout)
            : mAddressMask(x64::paging::addressMask(bits))
            , mHigherHalfOffset(hhdmOffset)
            , mLayout(layout)
        { }

        constexpr bool has3LevelPaging() const {
            return mAddressMask == x64::paging::addressMask(40);
        }

        constexpr bool has4LevelPaging() const {
            return mAddressMask == x64::paging::addressMask(48);
        }

        uintptr_t getAddressMask() const {
            return mAddressMask;
        }

        uint64_t hhdmOffset() const {
            return mHigherHalfOffset;
        }

        constexpr uintptr_t address(x64::Entry entry) const {
            return entry.underlying & mAddressMask;
        }

        constexpr void setAddress(x64::Entry& entry, uintptr_t address) const {
            entry.underlying = (entry.underlying & ~mAddressMask) | (address & mAddressMask);
        }

        constexpr void setMemoryType(x64::pte& entry, MemoryType type) const {
            entry.setPatEntry(getMemoryTypeIndex(type));
        }

        constexpr void setMemoryType(x64::pde& entry, MemoryType type) const {
            entry.setPatEntry(getMemoryTypeIndex(type));
        }

        constexpr void setMemoryType(x64::pdpte& entry, MemoryType type) const {
            entry.setPatEntry(getMemoryTypeIndex(type));
        }

        km::PhysicalAddress activeMap() const {
            return (x64::cr3() & mAddressMask);
        }

        void setActiveMap(km::PhysicalAddress map) const {
            uint64_t reg = x64::cr3();
            reg = (reg & ~mAddressMask) | map.address;
            x64::setcr3(reg);
        }
    };
}

template<>
struct km::StaticFormat<km::MemoryType> {
    static constexpr size_t kStringSize = 4;

    static constexpr stdx::StringView toString(km::MemoryType type) {
        using namespace stdx::literals;
        using enum km::MemoryType;

        switch (type) {
        case eUncached:
            return "UC"_sv;
        case eWriteCombine:
            return "WC"_sv;
        case eWriteThrough:
            return "WT"_sv;
        case eWriteProtect:
            return "WP"_sv;
        case eWriteBack:
            return "WB"_sv;
        case eUncachedOverridable:
            return "UC-"_sv;
        }
    }
};
