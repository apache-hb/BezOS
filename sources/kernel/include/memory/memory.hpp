#pragma once

#include <bezos/status.h>

#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"
#include "arch/paging.hpp"

#include "memory/layout.hpp"
#include "memory/range_allocator.hpp"
#include "memory/paging.hpp"

#include "std/spinlock.hpp"

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

    /// @brief Manages page tables for an address space.
    ///
    /// @details All ptes are allocated from a memory pool that is provided at construction.
    ///          The current implementation expects that the virtual and physical addresses have a fixed
    ///          offset between them.
    class PageTables {
        uintptr_t mSlide;
        mem::SynchronizedAllocator<mem::TlsfAllocator> mAllocator;
        const PageBuilder *mPageManager;
        x64::PageMapLevel4 *mRootPageTable;
        stdx::SpinLock mLock;

        x64::page *allocPage();

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address);

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, PageFlags flags);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, PageFlags flags);

        /// @brief Get the virtual address of a sub-table from a table entry.
        ///
        /// @pre @p table->entries[index].present() must be true.
        ///
        /// @tparam T The type of the table.
        /// @param table The table to get the entry from.
        /// @param index The index of the entry to get.
        /// @return The virtual address of the sub-table.
        template<typename U, typename T>
        auto *getPageEntry(const T *table, uint16_t index) const {
            uintptr_t address = mPageManager->address(table->entries[index]);
            return asVirtual<U>(address);
        }

        const x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        const x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        OsStatus mapRange4k(AddressMapping mapping, PageFlags flags, MemoryType type);
        OsStatus mapRange2m(AddressMapping mapping, PageFlags flags, MemoryType type);
        OsStatus mapRange1g(AddressMapping mapping, PageFlags flags, MemoryType type);

        x64::PageMapLevel4 *getRootTable() const {
            return mRootPageTable;
        }

        template<typename T>
        T *asVirtual(km::PhysicalAddress addr) const {
            return (T*)(addr.address + mSlide);
        }

        PhysicalAddress asPhysical(const void *ptr) const;

        OsStatus map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);
        OsStatus map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);
        OsStatus map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

    public:
        PageTables(AddressMapping pteMemory, const PageBuilder *pm);

        PhysicalAddress root() const {
            return asPhysical(mRootPageTable);
        }

        auto&& pml4(this auto&& self) {
            return self.mRootPageTable;
        }

        PhysicalAddress getBackingAddress(const void *ptr);
        PageFlags getMemoryFlags(const void *ptr);
        PageSize2 getPageSize(const void *ptr);

        OsStatus map(MappingRequest request);
        OsStatus unmap(VirtualRange range);

        OsStatus walk(const void *ptr, PageWalk *walk);
    };

    /// @brief Kernel address space.
    ///
    /// The top half of address space is reserved for the kernel.
    /// Each process has the kernel address space mapped into the higher half.
    /// Kernel addresses are identified by having the top bits set in the canonical address.
    class KernelPageTables {
        PageTables mSystemTables;
        RangeAllocator<const void*> mVmemAllocator;

    public:
        KernelPageTables(AddressMapping pteMemory, const km::PageBuilder *pm, VirtualRange systemArea)
            : mSystemTables(pteMemory, pm)
            , mVmemAllocator(systemArea)
        { }

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(AddressMapping mapping);

        PageTables& ptes() {
            return mSystemTables;
        }
    };

    /// @brief Per process address space.
    ///
    /// Each process has its own address space, which has the kernel address space mapped into the higher half.
    /// The lower half of the address space is reserved for the process.
    /// Process addresses are identified by having the top bits clear in the canonical address.
    class ProcessPageTables {
        KernelPageTables *mSystemTables;
        PageTables mProcessTables;
        RangeAllocator<const void*> mVmemAllocator;

    public:
        ProcessPageTables(KernelPageTables *kernel, AddressMapping pteMemory, const km::PageBuilder *pm, VirtualRange processArea)
            : mSystemTables(kernel)
            , mProcessTables(pteMemory, pm)
            , mVmemAllocator(processArea)
        {
            //
            // Copy the higher half mappings from the kernel ptes to the process ptes.
            //

            PageTables& system = mSystemTables->ptes();
            PageTables& process = mProcessTables;

            x64::PageMapLevel4 *pml4 = system.pml4();
            x64::PageMapLevel4 *self = process.pml4();

            //
            // Only copy the higher half mappings, which are 256-511.
            //
            static constexpr size_t kCount = (sizeof(x64::PageMapLevel4) / sizeof(x64::pml4e)) / 2;
            std::copy_n(pml4->entries + kCount, kCount, self->entries + kCount);
        }

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(AddressMapping mapping);
    };
}
