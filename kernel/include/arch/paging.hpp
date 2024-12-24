#pragma once

#include <stdint.h>

namespace x64 {
    inline void setmask(uint64_t& value, uint64_t mask, bool state) {
        if (state) {
            value |= mask;
        } else {
            value &= ~mask;
        }
    }

    constexpr uintptr_t kPageSize = 0x1000;

    namespace paging {
        constexpr uint64_t kMaxPhysicalAddress = 48;
        constexpr uint64_t kPresentBit        = 1ull << 0;
        constexpr uint64_t kReadOnlyBit       = 1ull << 1;
        constexpr uint64_t kUserBit           = 1ull << 2;
        constexpr uint64_t kAccessedBit       = 1ull << 5;
        constexpr uint64_t kWrittenBit        = 1ull << 6;
        constexpr uint64_t kExecuteDisableBit = 1ull << 63;

        constexpr bool isValidAddressWidth(uintptr_t width) {
            return width == 40 || width == 48;
        }

        constexpr uintptr_t addressMask(uintptr_t width) {
            return ((1ull << width) - 1) & ~(kPageSize - 1);
        }
    }

    struct Entry {
        uint64_t underlying;

        bool present() const { return underlying & paging::kPresentBit; }
        void setPresent(bool present) { setmask(underlying, paging::kPresentBit, present); }

        bool writeable() const { return (underlying & paging::kReadOnlyBit); }
        void setWriteable(bool readonly) { setmask(underlying, paging::kReadOnlyBit, readonly); }

        bool user() const { return underlying & paging::kUserBit; }
        void setUser(bool user) { setmask(underlying, paging::kUserBit, user); }

        bool accessed() const { return underlying & paging::kAccessedBit; }
        void setAccessed(bool accessed) { setmask(underlying, paging::kAccessedBit, accessed); }

        bool written() const { return underlying & paging::kWrittenBit; }
        void setWritten(bool written) { setmask(underlying, paging::kWrittenBit, written); }

        bool executable() const { return !(underlying & paging::kExecuteDisableBit); }
        void setExecutable(bool exec) { setmask(underlying, paging::kExecuteDisableBit, !exec); }
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
}
