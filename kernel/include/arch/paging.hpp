#pragma once

#include "arch/intrin.hpp"
#include <stdint.h>
#include <type_traits>

namespace x64 {
    inline void setmask(uint64_t& value, uint64_t mask, bool state) noexcept {
        if (state) {
            value |= mask;
        } else {
            value &= ~mask;
        }
    }

    static constexpr uintptr_t kPageSize = 0x1000;

    namespace paging {
        static constexpr uint64_t kMaxPhysicalAddress = 48;
        static constexpr uint64_t kPresentBit        = 1ull << 0;
        static constexpr uint64_t kReadOnlyBit       = 1ull << 1;
        static constexpr uint64_t kUserBit           = 1ull << 2;
        static constexpr uint64_t kAccessedBit       = 1ull << 5;
        static constexpr uint64_t kWrittenBit        = 1ull << 6;
        static constexpr uint64_t kExecuteDisableBit = 1ull << 63;

        constexpr uintptr_t kValidAddressBits[] = { 40, 48 };

        constexpr bool isValidAddressWidth(uintptr_t width) noexcept {
            for (auto bits : kValidAddressBits) {
                if (width == bits) {
                    return true;
                }
            }

            return false;
        }

        constexpr uintptr_t addressMask(uintptr_t width) noexcept {
            return ((1ull << width) - 1) & ~(kPageSize - 1);
        }
    }

    struct Entry {
        uint64_t underlying;

        bool present() const noexcept { return underlying & paging::kPresentBit; }
        void setPresent(bool present) noexcept { setmask(underlying, paging::kPresentBit, present); }

        bool writeable() const noexcept { return underlying & paging::kReadOnlyBit; }
        void setWriteable(bool readonly) noexcept { setmask(underlying, paging::kReadOnlyBit, readonly); }

        bool user() const noexcept { return underlying & paging::kUserBit; }
        void setUser(bool user) noexcept { setmask(underlying, paging::kUserBit, user); }

        bool accessed() const noexcept { return underlying & paging::kAccessedBit; }
        void setAccessed(bool accessed) noexcept { setmask(underlying, paging::kAccessedBit, accessed); }

        bool written() const noexcept { return underlying & paging::kWrittenBit; }
        void setWritten(bool written) noexcept { setmask(underlying, paging::kWrittenBit, written); }

        bool executable() const noexcept { return !(underlying & paging::kExecuteDisableBit); }
        void setExecutable(bool exec) noexcept { setmask(underlying, paging::kExecuteDisableBit, !exec); }
    };

    /// @brief Page table entry
    struct pte : Entry { };

    /// @brief Page directory entry
    struct pde : Entry { };

    /// @brief Page directory pointer table entry
    struct pdpte : Entry { };

    /// @brief Page map level 4 entry
    struct pml4e : Entry { };

    struct alignas(kPageSize) PageTable {
        pte entries[kPageSize / sizeof(pte)];
    };

    struct alignas(kPageSize) PageMapLevel2 {
        pde entries[kPageSize / sizeof(pde)];
    };

    struct alignas(kPageSize) PageMapLevel3 {
        pdpte entries[kPageSize / sizeof(pdpte)];
    };

    struct alignas(kPageSize) PageMapLevel4 {
        pml4e entries[kPageSize / sizeof(pml4e)];
    };

    struct alignas(kPageSize) page {
        uint8_t data[kPageSize];
    };

    class PageManager {
        uintptr_t mAddressMask;

    public:
        constexpr PageManager(uintptr_t bits) noexcept
            : mAddressMask(paging::addressMask(bits))
        { }

        constexpr bool has3LevelPaging() const noexcept {
            return mAddressMask == paging::addressMask(40);
        }

        constexpr bool has4LevelPaging() const noexcept {
            return mAddressMask == paging::addressMask(48);
        }

        uintptr_t getAddressMask() const noexcept {
            return mAddressMask;
        }

        constexpr uintptr_t address(Entry entry) const noexcept {
            return entry.underlying & mAddressMask;
        }

        constexpr void setAddress(Entry& entry, uintptr_t address) const noexcept {
            entry.underlying = (entry.underlying & ~mAddressMask) | (address & mAddressMask);
        }

        void *activeMap() const noexcept {
            return reinterpret_cast<void *>(cr3() & mAddressMask);
        }

        x64::PageMapLevel3 *activeMap3() const noexcept {
            return reinterpret_cast<x64::PageMapLevel3 *>(activeMap());
        }

        x64::PageMapLevel4 *activeMap4() const noexcept {
            return reinterpret_cast<x64::PageMapLevel4 *>(activeMap());
        }

        void setActiveMap(const void *map) const noexcept {
            uint64_t reg = cr3();
            reg = (reg & ~mAddressMask) | reinterpret_cast<uint64_t>(map);
            setcr3(reg);
        }

        void setActiveMap3(const x64::PageMapLevel3 *map) const noexcept {
            setActiveMap(map);
        }

        void setActiveMap4(const x64::PageMapLevel4 *map) const noexcept {
            setActiveMap(map);
        }
    };
}
