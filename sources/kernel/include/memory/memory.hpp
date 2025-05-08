#pragma once

#include <bezos/status.h>

#include "arch/paging.hpp"

#include "memory/layout.hpp"

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
        eUserAll = eAll | eUser,
    };

    UTIL_BITFLAGS(PageFlags);

    namespace detail {
        /// @brief Create the head of a large page mapping.
        /// @pre @p mapping must be @ref AlignLargeRangeEligible.
        AddressMapping AlignLargeRangeHead(AddressMapping mapping) noexcept [[clang::nonblocking]];

        /// @brief Create the body of a large page mapping.
        /// @pre @p mapping must be @ref AlignLargeRangeEligible.
        AddressMapping AlignLargeRangeBody(AddressMapping mapping) noexcept [[clang::nonblocking]];

        /// @brief Create the tail of a large page mapping.
        /// @pre @p mapping must be @ref AlignLargeRangeEligible.
        AddressMapping AlignLargeRangeTail(AddressMapping mapping) noexcept [[clang::nonblocking]];

        /// @brief Check if a mapping is eligible for large pages.
        bool IsLargePageEligible(AddressMapping mapping) noexcept [[clang::nonblocking]];

        /// @brief Calculate the maximum possible number of pages required to map a range of memory.
        ///
        /// This is used to determine the number of pages to pre-allocate when mapping memory to
        /// avoid partial mapping in low memory conditions. The implementation is closely tied to
        /// @ref PageTables::map and when that function is changed this function must be updated if
        /// the mapping strategy changes.
        ///
        /// @param range The range to map.
        /// @return The maximum number of pages required to map the range.
        size_t MaxPagesForMapping(VirtualRange range) noexcept [[clang::nonblocking]];

        size_t GetCoveredSegments(VirtualRange range, size_t segment) noexcept [[clang::nonblocking]];
    }

    enum class PageSize : uint8_t {
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

    constexpr PageWalkIndices GetAddressParts(uintptr_t address) noexcept [[clang::nonblocking]] {
        uint16_t pml4e = (address >> 39) & 0b0001'1111'1111;
        uint16_t pdpte = (address >> 30) & 0b0001'1111'1111;
        uint16_t pdte = (address >> 21) & 0b0001'1111'1111;
        uint16_t pte = (address >> 12) & 0b0001'1111'1111;

        return PageWalkIndices { pml4e, pdpte, pdte, pte };
    }

    PageWalkIndices GetAddressParts(const void *ptr) noexcept [[clang::nonblocking]];

    /// @brief The result of walking page tables to a virtual address.
    ///
    /// @note If any address is not found, all addresses afterwards will also be invalid.
    struct PageWalk {
        /// @brief The virtual address that was walked.
        const void *address;

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

        constexpr PageSize pageSize() const noexcept [[clang::nonblocking]] {
            if (pdpte.present() && pdpte.is1g()) return PageSize::eHuge;
            if (pdte.present() && pdte.is2m()) return PageSize::eLarge;
            if (pte.present()) return PageSize::eRegular;
            return PageSize::eNone;
        }

        constexpr PageFlags flags() const noexcept [[clang::nonblocking]] {
            PageFlags flags = PageFlags::eUserAll;

            auto applyFlags = [&](auto entry) {
                if (!entry.present()) return;

                if (!entry.writeable()) flags &= ~PageFlags::eWrite;
                if (!entry.executable()) flags &= ~PageFlags::eExecute;
                if (!entry.user()) flags &= ~PageFlags::eUser;
            };

            applyFlags(pml4e);

            if (!pdpte.present()) return PageFlags::eNone;
            applyFlags(pdpte);
            if (pdpte.is1g()) return flags;

            if (!pdte.present()) return PageFlags::eNone;
            applyFlags(pdte);
            if (pdte.is2m()) return flags;

            if (!pte.present()) return PageFlags::eNone;
            applyFlags(pte);

            return flags;
        }
    };
}

template<>
struct km::Format<km::PageSize> {
    static void format(km::IOutStream& out, km::PageSize value) {
        switch (value) {
        case km::PageSize::eNone: out.format("None"); break;
        case km::PageSize::eRegular: out.format("4k"); break;
        case km::PageSize::eLarge: out.format("2m"); break;
        case km::PageSize::eHuge: out.format("1g"); break;
        }
    }
};

template<>
struct km::Format<km::PageFlags> {
    static void format(km::IOutStream& out, km::PageFlags value) {
        if (bool(value & km::PageFlags::eRead)) out.format("R");
        if (bool(value & km::PageFlags::eWrite)) out.format("W");
        if (bool(value & km::PageFlags::eExecute)) out.format("X");

        if (bool(value & km::PageFlags::eUser))
            out.format("U");
        else
            out.format("S");
    }
};
