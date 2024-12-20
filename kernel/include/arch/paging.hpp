#pragma once

#include <stdint.h>

namespace x64 {
    inline void setmask(uint64_t& value, uint64_t mask, bool state) noexcept {
        if (state) {
            value |= mask;
        } else {
            value &= ~mask;
        }
    }

    namespace paging {
        static constexpr uint64_t kMaxPhysicalAddress = 48;
        static constexpr uint64_t kPresentBit        = 1ull << 0;
        static constexpr uint64_t kReadOnlyBit       = 1ull << 1;
        static constexpr uint64_t kUserBit           = 1ull << 2;
        static constexpr uint64_t kAccessedBit       = 1ull << 5;
        static constexpr uint64_t kWrittenBit        = 1ull << 6;
        static constexpr uint64_t kExecuteDisableBit = 1ull << 63;

        static constexpr uintptr_t kAddressMask = ((1ull << kMaxPhysicalAddress) - 1) & ~((1ull << 12) - 1);
    }

    static constexpr uintptr_t kPageSize = 0x1000;

    /// @brief Page table entry
    struct pte {
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

        uintptr_t address() const noexcept { return underlying & paging::kAddressMask; }
        void setAddress(uintptr_t address) noexcept { underlying = (underlying & ~paging::kAddressMask) | (address & paging::kAddressMask); }
    };

    /// @brief Page directory entry
    struct pde {
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

        uintptr_t address() const noexcept { return underlying & paging::kAddressMask; }
        void setAddress(uintptr_t address) noexcept { underlying = (underlying & ~paging::kAddressMask) | (address & paging::kAddressMask); }
    };

    /// @brief Page directory pointer table entry
    struct pdpte {
        uint64_t underlying;

        bool present() const noexcept { return underlying & paging::kPresentBit; }
        void setPresent(bool present) noexcept { setmask(underlying, paging::kPresentBit, present); }

        bool writeable() const noexcept { return underlying & paging::kReadOnlyBit; }
        void setWriteable(bool readonly) noexcept { setmask(underlying, paging::kReadOnlyBit, readonly); }

        bool user() const noexcept { return underlying & paging::kUserBit; }
        void setUser(bool user) noexcept { setmask(underlying, paging::kUserBit, user); }

        bool accessed() const noexcept { return underlying & paging::kAccessedBit; }
        void setAccessed(bool accessed) noexcept { setmask(underlying, paging::kAccessedBit, accessed); }

        bool executable() const noexcept { return !(underlying & paging::kExecuteDisableBit); }
        void setExecutable(bool exec) noexcept { setmask(underlying, paging::kExecuteDisableBit, !exec); }

        uintptr_t address() const noexcept { return underlying & paging::kAddressMask; }
        void setAddress(uintptr_t address) noexcept { underlying = (underlying & ~paging::kAddressMask) | (address & paging::kAddressMask); }
    };

    /// @brief Page map level 4 entry
    struct pml4e {
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

        uintptr_t address() const noexcept { return underlying & paging::kAddressMask; }
        void setAddress(uintptr_t address) noexcept { underlying = (underlying & ~paging::kAddressMask) | (address & paging::kAddressMask); }
    };

    struct alignas(0x1000) PageTable {
        pte entries[0x1000 / sizeof(pte)];
    };

    struct alignas(0x1000) PageMapLevel2 {
        pde entries[0x1000 / sizeof(pde)];
    };

    struct alignas(0x1000) PageMapLevel3 {
        pdpte entries[0x1000 / sizeof(pdpte)];
    };

    struct alignas(0x1000) PageMapLevel4 {
        pml4e entries[0x1000 / sizeof(pml4e)];
    };
}
