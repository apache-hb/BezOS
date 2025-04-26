#pragma once

#include "arch/intrin.hpp"
#include "util/memory.hpp"

#include <stdint.h>

namespace x64 {
    constexpr void setmask(uint64_t& value, uint64_t mask, bool state) noexcept [[clang::nonblocking]] {
        if (state) {
            value |= mask;
        } else {
            value &= ~mask;
        }
    }

    constexpr uintptr_t kPageSize = sm::kilobytes(4).bytes();
    constexpr uintptr_t kLargePageSize = sm::megabytes(2).bytes();
    constexpr uintptr_t kHugePageSize = sm::gigabytes(1).bytes();

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

        static_assert(addressMask(40) == 0x0000'00ff'ffff'f000ull);
        static_assert(addressMask(48) == 0x0000'ffff'ffff'f000ull);

        // See Vol 3A Table 13-11. Selection of PAT Entries with PAT, PCD, and PWT Flags
        constexpr void setMemoryType(uint64_t& entry, uint64_t pat, uint8_t type) {
            uint64_t mask
                = kWriteThroughBit
                | kCacheDisableBit
                | (1ull << pat);

            uint64_t select
                = (type & 0b0000'0001) << 3
                | (type & 0b0000'0010) << 3
                | (type & 0b0000'0100) << (pat - 2);

            entry = (entry & ~mask) | select;
        }

        constexpr uint8_t getMemoryType(uint64_t entry, uint64_t pat) {
            return (entry & kWriteThroughBit) >> 3
                | (entry & kCacheDisableBit) >> 3
                | (entry & (1ull << pat)) >> (pat - 2);
        }
    }

    struct Entry {
        uint64_t underlying;

        bool present() const noexcept [[clang::nonblocking]] { return underlying & paging::kPresentBit; }
        void setPresent(bool present) noexcept [[clang::nonblocking]] { setmask(underlying, paging::kPresentBit, present); }

        bool writeable() const noexcept [[clang::nonblocking]] { return (underlying & paging::kReadOnlyBit); }
        void setWriteable(bool readonly) noexcept [[clang::nonblocking]] { setmask(underlying, paging::kReadOnlyBit, readonly); }

        bool writeThrough() const { return underlying & paging::kWriteThroughBit; }
        void setWriteThrough(bool writeThrough) { setmask(underlying, paging::kWriteThroughBit, writeThrough); }

        bool cacheDisable() const { return underlying & paging::kCacheDisableBit; }
        void setCacheDisable(bool cacheDisable) { setmask(underlying, paging::kCacheDisableBit, cacheDisable); }

        bool user() const noexcept [[clang::nonblocking]] { return underlying & paging::kUserBit; }
        void setUser(bool user) noexcept [[clang::nonblocking]] { setmask(underlying, paging::kUserBit, user); }

        bool accessed() const { return underlying & paging::kAccessedBit; }
        void setAccessed(bool accessed) { setmask(underlying, paging::kAccessedBit, accessed); }

        bool written() const { return underlying & paging::kWrittenBit; }
        void setWritten(bool written) { setmask(underlying, paging::kWrittenBit, written); }

        bool executable() const noexcept [[clang::nonblocking]] { return !(underlying & paging::kExecuteDisableBit); }
        void setExecutable(bool exec) noexcept [[clang::nonblocking]] { setmask(underlying, paging::kExecuteDisableBit, !exec); }
    };

    /// @brief Page table entry
    struct pte : Entry {
        static constexpr uint64_t kPageAttributeBit = 7;
        static constexpr uintptr_t kAddressMask = ~((1ull << 12) - 1);

        /// @brief Set the PAT entry to use for this page
        void setPatEntry(uint8_t entry) noexcept [[clang::nonblocking]] {
            // See Vol 3A 5-31. Table 5-20. Format of a Page-Table Entry that Maps a 4-KByte Page
            // The PAT is encoded into bits 3 (PWT), 4 (PCD), and 7 (PAT)
            paging::setMemoryType(underlying, kPageAttributeBit, entry);
        }

        uint8_t memoryType() const noexcept [[clang::nonblocking]] {
            return paging::getMemoryType(underlying, kPageAttributeBit);
        }
    };

    /// @brief Page directory entry
    struct pdte : Entry {
        static constexpr uint64_t kLargePage = 1ull << 7;
        static constexpr uintptr_t kAddressMask = ~((1ull << 21) - 1);

        bool is2m() const noexcept [[clang::nonblocking]] { return underlying & kLargePage; }
        void set2m(bool large) noexcept [[clang::nonblocking]] { setmask(underlying, kLargePage, large); }

        static constexpr uint64_t kPageAttributeBit = 12;

        /// @brief Set the PAT entry to use for this page
        void setPatEntry(uint8_t entry) noexcept [[clang::nonblocking]] {
            // See Vol 3A 5-29. Table 5-18. Format of a Page-Directory Entry that Maps a 2-MByte Page
            paging::setMemoryType(underlying, kPageAttributeBit, entry);
        }

        uint8_t memoryType() const noexcept [[clang::nonblocking]] {
            return paging::getMemoryType(underlying, kPageAttributeBit);
        }
    };

    /// @brief Page directory pointer table entry
    struct pdpte : Entry {
        static constexpr uint64_t kHugePage = 1ull << 7;
        static constexpr uintptr_t kAddressMask = ~((1ull << 30) - 1);

        bool is1g() const noexcept [[clang::nonblocking]] { return underlying & kHugePage; }
        void set1g(bool huge) noexcept [[clang::nonblocking]] { setmask(underlying, kHugePage, huge); }

        static constexpr uint64_t kPageAttributeBit = 12;

        /// @brief Set the PAT entry to use for this page
        void setPatEntry(uint8_t entry) noexcept [[clang::nonblocking]] {
            // See Vol 3A 5-27.Table 5-16. Format of a Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page
            paging::setMemoryType(underlying, kPageAttributeBit, entry);
        }

        uint8_t memoryType() const noexcept [[clang::nonblocking]] {
            return paging::getMemoryType(underlying, kPageAttributeBit);
        }
    };

    /// @brief Page map level 4 entry
    struct pml4e : Entry { };

    struct alignas(kPageSize) PageTable {
        pte entries[kPageSize / sizeof(pte)];
    };

    struct alignas(kPageSize) PageMapLevel2 {
        pdte entries[kPageSize / sizeof(pdte)];
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

    [[gnu::always_inline, gnu::nodebug]]
    static inline void invlpg([[maybe_unused]] uintptr_t address) noexcept [[clang::nonallocating]] {
#if __STDC_HOSTED__ == 0
        arch::Intrin::invlpg(address);
#endif
    }
}
