#pragma once

#include <bezos/status.h>

#include "arch/paging.hpp"

#include "memory/layout.hpp"
#include "memory/paging.hpp"

namespace km {
    enum class PageFlags : uint8_t {
        eNone = 0,
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,
        eUser = 1 << 3,

        eCode = eRead | eExecute,
        eData = eRead | eWrite,
        eAll = eRead | eWrite | eExecute,

        eUserData = eData | eUser,
        eUserCode = eCode | eUser,

        eUserAll = eRead | eWrite | eExecute | eUser,
    };

    UTIL_BITFLAGS(PageFlags);

    namespace detail {
        /// @brief Create the head of a large page mapping.
        /// @pre @p mapping must be @ref AlignLargeRangeEligible.
        AddressMapping AlignLargeRangeHead(AddressMapping mapping);
        AddressMapping AlignLargeRangeBody(AddressMapping mapping);
        AddressMapping AlignLargeRangeTail(AddressMapping mapping);

        bool IsLargePageEligible(AddressMapping mapping);
    }

    struct MappingRequest {
        AddressMapping mapping;
        PageFlags flags = PageFlags::eNone;
        MemoryType type = MemoryType::eWriteBack;
    };

    enum class PageSize2 {
        eNone,
        eRegular,
        eLarge,
        eHuge,
    };

    struct PageWalkIndices {
        uint16_t pml4e;
        uint16_t pdpte;
        uint16_t pdte;
        uint16_t pte;
    };

    constexpr PageWalkIndices GetAddressParts(uintptr_t address) {
        uint16_t pml4e = (address >> 39) & 0b0001'1111'1111;
        uint16_t pdpte = (address >> 30) & 0b0001'1111'1111;
        uint16_t pdte = (address >> 21) & 0b0001'1111'1111;
        uint16_t pte = (address >> 12) & 0b0001'1111'1111;

        return PageWalkIndices { pml4e, pdpte, pdte, pte };
    }

    PageWalkIndices GetAddressParts(const void *ptr);

    /// @brief The result of walking page tables to a virtual address.
    ///
    /// @note If any address is not found, all addresses afterwards will also be invalid.
    struct PageWalk {
        /// @brief The virtual address that was walked.
        uintptr_t address;

        /// @brief The entry in the pml4 that contains the address.
        uint16_t pml4eIndex;

        /// @brief The value of the pml4 entry.
        /// @note If @a pml4e->present() is false, the address is invalid.
        x64::pml4e pml4e;

        /// @brief The entry in the pdpt that contains the address.
        uint16_t pdpteIndex;

        /// @brief The value of the pdpte entry.
        /// @note If @a pdpte->present() is false, the address is invalid.
        /// @note If @a pdpte->is1g() is true, all following fields are invalid.
        x64::pdpte pdpte;

        /// @brief The entry in the pdt that contains the address.
        uint16_t pdteIndex;

        /// @brief The address of the pt that the pdte points to.
        /// @note If @a pdte->present() is false, the address is invalid.
        /// @note If @a pdte->is2m() is true, all following fields are invalid.
        x64::pdte pdte;

        /// @brief The entry in the pt that contains the address.
        uint16_t pteIndex;

        /// @brief The address of the page that the pte points to.
        /// @note If @a pte->present() is false, the address is invalid.
        x64::pte pte;

        constexpr PageSize2 pageSize() const {
            if (pdpte.present() && pdpte.is1g()) return PageSize2::eHuge;
            if (pdte.present() && pdte.is2m()) return PageSize2::eLarge;
            if (pte.present()) return PageSize2::eRegular;
            return PageSize2::eNone;
        }

        constexpr PageFlags flags() const {
            PageFlags flags = PageFlags::eUserAll;

            auto applyFlags = [&](auto entry) {
                if (!entry.present()) return;

                if (!entry.writeable()) flags &= ~PageFlags::eWrite;
                if (!entry.executable()) flags &= ~PageFlags::eExecute;
                if (!entry.user()) flags &= ~PageFlags::eUser;
            };

            applyFlags(pml4e);
            applyFlags(pdpte);
            applyFlags(pdte);
            applyFlags(pte);

            return flags;
        }
    };
}
