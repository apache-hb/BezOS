#pragma once

#include "util/memory.hpp"

#include <stdint.h>

namespace x64 {
    constexpr void setmask(uint64_t& value, uint64_t mask, bool state) {
        if (state) {
            value |= mask;
        } else {
            value &= ~mask;
        }
    }

    constexpr uintptr_t kPageSize = sm::kilobytes(4).asBytes();
    constexpr uintptr_t kLargePageSize = sm::megabytes(2).asBytes();

    namespace paging {
        constexpr uint64_t kMaxPhysicalAddress = 48;
        constexpr uint64_t kPresentBit        = 1ull << 0;
        constexpr uint64_t kReadOnlyBit       = 1ull << 1;
        constexpr uint64_t kUserBit           = 1ull << 2;
        constexpr uint64_t kWriteThroughBit   = 1ull << 3;
        constexpr uint64_t kCacheDisableBit   = 1ull << 4;
        constexpr uint64_t kAccessedBit       = 1ull << 5;
        constexpr uint64_t kWrittenBit        = 1ull << 6;
        constexpr uint64_t kExecuteDisableBit = 1ull << 63;

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

        bool writeThrough() const { return underlying & paging::kWriteThroughBit; }
        void setWriteThrough(bool writeThrough) { setmask(underlying, paging::kWriteThroughBit, writeThrough); }

        bool cacheDisable() const { return underlying & paging::kCacheDisableBit; }
        void setCacheDisable(bool cacheDisable) { setmask(underlying, paging::kCacheDisableBit, cacheDisable); }

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
    struct pte : Entry {
        static constexpr uint64_t kPageAttributeBit = 7;

        /// @brief Set the PAT entry to use for this page
        void setPatEntry(uint8_t entry) {
            // See Vol 3, 13.12.3 Table 13-11. Selection of PAT Entries with PAT, PCD, and PWT Flags
            // The PAT is encoded into bits 3 (PWT), 4 (PCD), and 7 (PAT)
            static constexpr uint64_t mask
                = paging::kWriteThroughBit
                | paging::kCacheDisableBit
                | (1ull << kPageAttributeBit);

            // See Vol 3A 5-31. Table 5-20. Format of a Page-Table Entry that Maps a 4-KByte Page
            // subtract from each elements slide to account for bit positions.
            uint64_t select
                = (entry & 0b0000'0001) << (3 - 0)
                | (entry & 0b0000'0010) << (4 - 1)
                | (entry & 0b0000'0100) << (kPageAttributeBit - 2);

            underlying = (underlying & ~mask) | select;
        }
    };

    /// @brief Page directory entry
    struct pde : Entry {
        static constexpr uint64_t kLargePage = 1ull << 7;
        bool is2m() const { return underlying & kLargePage; }
        void set2m(bool large) { setmask(underlying, kLargePage, large); }

        static constexpr uint64_t kPageAttributeBit = 12;

        /// @brief Set the PAT entry to use for this page
        void setPatEntry(uint8_t entry) {
            static constexpr uint64_t mask
                = paging::kWriteThroughBit
                | paging::kCacheDisableBit
                | (1ull << kPageAttributeBit);

            // See Vol 3A 5-29. Table 5-18. Format of a Page-Directory Entry that Maps a 2-MByte Page
            uint64_t select
                = (entry & 0b0000'0001) << (3 - 0)
                | (entry & 0b0000'0010) << (4 - 1)
                | (entry & 0b0000'0100) << (kPageAttributeBit - 2);

            underlying = (underlying & ~mask) | select;
        }
    };

    /// @brief Page directory pointer table entry
    struct pdpte : Entry {
        static constexpr uint64_t kHugePage = 1ull << 7;

        bool is1g() const { return underlying & kHugePage; }
        void set1g(bool huge) { setmask(underlying, kHugePage, huge); }

        static constexpr uint64_t kPageAttributeBit = 12;

        /// @brief Set the PAT entry to use for this page
        void setPatEntry(uint8_t entry) {
            static constexpr uint64_t mask
                = paging::kWriteThroughBit
                | paging::kCacheDisableBit
                | (1ull << kPageAttributeBit);

            // See Vol 3A 5-27.Table 5-16. Format of a Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page
            uint64_t select
                = (entry & 0b0000'0001) << (3 - 0)
                | (entry & 0b0000'0010) << (4 - 1)
                | (entry & 0b0000'0100) << (kPageAttributeBit - 2);

            underlying = (underlying & ~mask) | select;
        }
    };

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
