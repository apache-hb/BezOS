#pragma once

#include "arch/cr3.hpp"
#include "arch/intrin.hpp"
#include "arch/paging.hpp"

#include "memory/range.hpp"
#include "util/format.hpp"

namespace km {
    /// @brief Page Attribute Table (PAT) and Memory Type Range Registers (MTRRs) choices.
    /// @see AMD64 Architecture Programmer's Manual Volume 2: System Programming Table 7-9.
    enum class MemoryType : uint8_t {
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

    namespace detail {
        constexpr uint8_t GetMemoryPatIndex(const PageMemoryTypeLayout& layout, MemoryType type) noexcept [[clang::nonblocking]] {
            switch (type) {
            case MemoryType::eUncached:
                return layout.uncached;
            case MemoryType::eWriteCombine:
                return layout.writeCombined;
            case MemoryType::eWriteThrough:
                return layout.writeThrough;
            case MemoryType::eWriteProtect:
                return layout.writeProtect;
            case MemoryType::eWriteBack:
                return layout.writeBack;
            default:
                return layout.deferred;
            }
        }

        constexpr MemoryType GetMemoryType(const PageMemoryTypeLayout& layout, uint8_t index) noexcept [[clang::nonblocking]] {
            if (layout.deferred == index) return MemoryType::eUncachedOverridable;
            if (layout.uncached == index) return MemoryType::eUncached;
            if (layout.writeCombined == index) return MemoryType::eWriteCombine;
            if (layout.writeThrough == index) return MemoryType::eWriteThrough;
            if (layout.writeProtect == index) return MemoryType::eWriteProtect;
            if (layout.writeBack == index) return MemoryType::eWriteBack;

            return MemoryType::eUncachedOverridable;
        }
    }

    class PageBuilder {
        uintptr_t mAddressMask;
        uintptr_t mVaddrBits;
        PageMemoryTypeLayout mLayout;

        constexpr uint8_t getMemoryTypeIndex(MemoryType type) const {
            return detail::GetMemoryPatIndex(mLayout, type);
        }

    public:
        constexpr PageBuilder() noexcept = default;

        constexpr PageBuilder(uintptr_t paddrbits, uintptr_t vaddrbits, PageMemoryTypeLayout layout)
            : mAddressMask(x64::paging::addressMask(paddrbits))
            , mVaddrBits(vaddrbits)
            , mLayout(layout)
        { }

        constexpr bool has4LevelPaging() const {
            return mAddressMask == x64::paging::addressMask(48);
        }

        constexpr PhysicalAddress maxPhysicalAddress() const {
            return PhysicalAddress { mAddressMask };
        }

        constexpr uintptr_t getAddressMask() const {
            return mAddressMask;
        }

        constexpr uintptr_t address(x64::Entry entry) const noexcept [[clang::nonallocating]] {
            return entry.underlying & mAddressMask;
        }

        constexpr void setAddress(x64::Entry& entry, uintptr_t address) const {
            entry.underlying = (entry.underlying & ~mAddressMask) | (address & mAddressMask);
        }

        constexpr void setAddress(x64::Entry& entry, PhysicalAddress address) const {
            setAddress(entry, address.address);
        }

        constexpr void setMemoryType(x64::pte& entry, MemoryType type) const {
            entry.setPatEntry(getMemoryTypeIndex(type));
        }

        constexpr void setMemoryType(x64::pdte& entry, MemoryType type) const {
            entry.setPatEntry(getMemoryTypeIndex(type));
        }

        constexpr void setMemoryType(x64::pdpte& entry, MemoryType type) const {
            entry.setPatEntry(getMemoryTypeIndex(type));
        }

        constexpr MemoryType getMemoryType(x64::pte entry) const {
            return static_cast<MemoryType>(entry.memoryType());
        }

        constexpr MemoryType getMemoryType(x64::pdte entry) const {
            return static_cast<MemoryType>(entry.memoryType());
        }

        constexpr MemoryType getMemoryType(x64::pdpte entry) const {
            return static_cast<MemoryType>(entry.memoryType());
        }

        PhysicalAddress activeMap() const noexcept {
            return (__get_cr3() & mAddressMask);
        }

        void setActiveMap(PhysicalAddressEx map) const noexcept {
            x64::Cr3 cr3 = x64::Cr3::load();
            cr3.setAddress(map.address & mAddressMask);
            x64::Cr3::store(cr3);
        }

        constexpr bool isCanonicalAddress(uintptr_t addr) const noexcept [[clang::nonblocking]] {
            uintptr_t mask = -(1ull << (mVaddrBits - 1));
            uintptr_t bits = (addr & mask);

            // bits must either be all 0 or all 1
            return bits == 0 || bits == mask;
        }

        constexpr bool isHigherHalf(uintptr_t addr) const noexcept [[clang::nonblocking]] {
            uintptr_t mask = -(1ull << (mVaddrBits - 1));
            uintptr_t bits = (addr & mask);

            // higher half requires all the high bits to be set
            return bits == mask;
        }

        bool isCanonicalAddress(const void *ptr) const noexcept [[clang::nonblocking]] {
            return isCanonicalAddress(reinterpret_cast<uintptr_t>(ptr));
        }

        bool isHigherHalf(const void *ptr) const noexcept [[clang::nonblocking]] {
            return isHigherHalf(reinterpret_cast<uintptr_t>(ptr));
        }
    };
}

template<>
struct km::Format<km::MemoryType> {
    static constexpr size_t kStringSize = 4;

    static void format(km::IOutStream& out, km::MemoryType value) {
        using enum km::MemoryType;

        switch (value) {
        case eUncached:
            out.write("UC");
            break;
        case eWriteCombine:
            out.write("WC");
            break;
        case eWriteThrough:
            out.write("WT");
            break;
        case eWriteProtect:
            out.write("WP");
            break;
        case eWriteBack:
            out.write("WB");
            break;
        case eUncachedOverridable:
            out.write("UC-");
            break;
        default:
            out.write(km::Hex(std::to_underlying(value)).pad(2));
            break;
        }
    }
};
