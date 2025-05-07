#pragma once

#include "arch/paging.hpp"

#include "memory/detail/table_list.hpp"

#include "memory/memory.hpp"
#include "memory/paging.hpp"
#include "memory/range.hpp"
#include "memory/table_allocator.hpp"
#include "std/spinlock.hpp"

namespace km {
    class PageTableCommandList;

    /// @brief Manages page tables for an address space.
    ///
    /// @details All ptes are allocated from a memory pool that is provided at construction.
    ///          The current implementation expects that the virtual and physical addresses have a fixed
    ///          offset between them.
    class PageTables {
        friend class PageTableCommandList;

        uintptr_t mSlide;
        PageTableAllocator mAllocator;
        const PageBuilder *mPageManager;
        x64::PageMapLevel4 *mRootPageTable;
        PageFlags mMiddleFlags;

        // TODO: remove lock, should be externally synchronized
        stdx::SpinLock mLock;

        /// @brief Allocate a new page table, garanteed to be aligned to 4k and zeroed.
        ///
        /// @return The new page table, or @c nullptr if no memory is available.
        x64::page *alloc4k() [[clang::allocating]];

        /// @brief Convert the physical address of a page table to a virtual address.
        ///
        /// All page tables are allocated from a memory pool which has a fixed offset.
        /// We can use that to convert between physical and virtual addresses.
        ///
        /// @param addr The physical address of the page table.
        /// @return The virtual address of the page table.
        template<typename T>
        T *asVirtual(PhysicalAddress addr) const noexcept [[clang::nonblocking]] {
            return (T*)(addr.address + mSlide);
        }

        /// @brief Convert the virtual address of a page table to a physical address.
        ///
        /// This can be used for storing a new page table in a pte.
        ///
        /// @param ptr The virtual address of the page table.
        /// @return The physical address of the page table.
        PhysicalAddress asPhysical(const void *ptr) const noexcept [[clang::nonallocating]];

        /// @brief Get the virtual address of a sub-table from a table entry.
        ///
        /// @pre @p table->entries[index].present() must be true.
        ///
        /// @tparam T The type of the table.
        /// @param table The table to get the entry from.
        /// @param index The index of the entry to get.
        /// @return The virtual address of the sub-table.
        template<typename U, typename T>
        auto *getPageEntry(const T *table, uint16_t index) const noexcept [[clang::nonallocating]] {
            PhysicalAddress address = mPageManager->address(table->entries[index]);
            return asVirtual<U>(address);
        }

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) noexcept [[clang::nonblocking]];

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];

        x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        void mapRange4k(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];
        void mapRange2m(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];
        void mapRange1g(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer);

        void map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];
        void map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];
        void map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type, detail::PageTableList& buffer);

        void partialRemap2m(x64::PageTable *pt, VirtualRange range, PhysicalAddress paddr, MemoryType type);

        /// @brief Split a 2m page mapping into 4k pages.
        ///
        /// @pre @p pde.is2m() must be true.
        /// @pre @p page.contains(erase) must be true.
        ///
        /// @param pde The 2m page directory entry to split.
        /// @param page The range of the 2m page.
        /// @param erase The range to unmap.
        /// @param pt The newly allocated page table.
        void split2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase, x64::PageTable *pt);

        /// @brief Cut off one end of a 2m page mapping and replace it with 4k pages.
        ///
        /// @pre @p pde.is2m() must be true.
        /// @pre @p page.intersects(erase) must be true.
        ///
        /// @param pde The 2m page directory entry to split.
        /// @param page The range of the 2m page.
        /// @param erase The range to unmap.
        /// @param pt The new page table to use.
        void cut2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase, x64::PageTable *pt);

        /// @brief Compute the number of page tables that need to be allocated to unmap the given range.
        ///
        /// @param range The range to unmap.
        ///
        /// @return The number of page tables required, either 0, 1, or 2.
        int earlyAllocatePageTables(VirtualRange range) noexcept [[clang::nonallocating]];

        x64::pdte& getLargePageEntry(const void *address) noexcept [[clang::nonallocating]];

        PageWalk walkUnlocked(const void *ptr) const;

        OsStatus earlyUnmap(VirtualRange range, VirtualRange *remaining);
        void earlyUnmapWithList(int earlyAllocations, VirtualRange range, VirtualRange *remaining, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];

        PhysicalAddress getBackingAddressUnlocked(const void *ptr) const;

        void reclaim2m(x64::pdte& pde);

        /// @brief Calculate the worst case memory usage for mapping a range.
        size_t maxPagesForMapping(VirtualRange range) const;

        /// @brief Walk the page tables to calculate the number of pages required to map the given range.
        size_t countPagesForMapping(VirtualRange range);

        OsStatus allocatePageTables(VirtualRange range, detail::PageTableList *list);

        /// @brief Private API for @a km::PageTableCommandList.
        OsStatus reservePageTablesForMapping(VirtualRange range, detail::PageTableList& list);

        /// @brief Private API for @a km::PageTableCommandList.
        OsStatus reservePageTablesForUnmapping(VirtualRange range, detail::PageTableList& list);

        /// @brief Private API for @a km::PageTableCommandList.
        void drainTableList(detail::PageTableList list) noexcept [[clang::nonallocating]];

        size_t compactUnlocked();

        size_t compactPt(x64::PageMapLevel2 *pd, uint16_t index);
        size_t compactPd(x64::PageMapLevel3 *pd, uint16_t index);
        size_t compactPdpt(x64::PageMapLevel4 *pd, uint16_t index);
        size_t compactPml4(x64::PageMapLevel4 *pml4);

        void mapWithList(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];
        bool verifyMapping(AddressMapping mapping) const noexcept [[clang::nonblocking]];

        /// @brief Private API for @a km::PageTableCommandList.
        void unmapWithList(VirtualRange range, detail::PageTableList& buffer) noexcept [[clang::nonallocating]];
        void unmapUnlocked(VirtualRange range) noexcept [[clang::nonallocating]];

        PageTables(const PageBuilder *pm, uintptr_t slide, PageTableAllocator&& allocator, x64::PageMapLevel4 *root, PageFlags middleFlags) noexcept [[clang::nonblocking]];
    public:
        constexpr PageTables() noexcept [[clang::nonblocking]] = default;

        constexpr PageTables(PageTables&& other) noexcept
            : mSlide(other.mSlide)
            , mAllocator(std::move(other.mAllocator))
            , mPageManager(other.mPageManager)
            , mRootPageTable(other.mRootPageTable)
            , mMiddleFlags(other.mMiddleFlags)
        { }

        constexpr PageTables& operator=(PageTables&& other) noexcept {
            mSlide = other.mSlide;
            mAllocator = std::move(other.mAllocator);
            mPageManager = other.mPageManager;
            mRootPageTable = other.mRootPageTable;
            mMiddleFlags = other.mMiddleFlags;
            return *this;
        }

        const PageBuilder *pageManager() const noexcept { return mPageManager; }

        const x64::PageMapLevel4 *pml4() const noexcept [[clang::nonblocking]] { return mRootPageTable; }
        x64::PageMapLevel4 *pml4() noexcept [[clang::nonblocking]] { return mRootPageTable; }
        PhysicalAddress root() const noexcept { return asPhysical(pml4()); }

        /// @brief Map a range of virtual address space to physical memory.
        ///
        /// @param mapping The mapping to create.
        /// @param flags The flags to use for the mapping.
        /// @param type The type of memory to map.
        ///
        /// @retval OsStatusSuccess The mapping was successful, the full range is mapped.
        /// @retval OsStatusOutOfMemory There was not enough memory to map the range, no memory has been mapped.
        /// @retval OsStatusInvalidInput The address mapping was malformed, no memory has been mapped.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus map(AddressMapping mapping, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        /// @brief Unmap a range of memory.
        ///
        /// Unmaps the given range of memory, if any part of the range is not already mapped it is ignored.
        /// Can partially unmap larger pages and will allocate new page tables as needed, hence the unmap
        /// operation can fail if memory is exhausted.
        ///
        /// @note This operation can fail with @a OsStatusOutOfMemory if @p range is not aligned to 2m and is
        ///       partially unmapping a 2m page. In this case the unmap will fail and no memory will be unmapped.
        ///
        /// @param range The range to unmap.
        ///
        /// @retval OsStatusSuccess The unmap was successful, the full range is unmapped.
        /// @retval OsStatusOutOfMemory There was not enough memory to allocate subtables, no memory has been unmapped.
        /// @retval OsStatusInvalidInput The address mapping was malformed, no memory has been unmapped.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus unmap(VirtualRange range);

        /// @brief Unmap a 2m aligned range of memory.
        ///
        /// Unmaps the given 2m aligned range of memory, if any part of the range is not already mapped it is ignored.
        /// Unlike @c unmap this function will not allocate new page tables, but as a result @p range must be 2m aligned.
        ///
        /// @param range The range to unmap.
        ///
        /// @retval OsStatusSuccess The unmap was successful, the full range is unmapped.
        /// @retval OsStatusInvalidInput The address mapping was malformed or not aligned to 2m, no memory has been unmapped.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus unmap2m(VirtualRange range);

        /// @brief Walk the page tables to find all tables involved in mapping a given address.
        ///
        /// @param ptr The address to walk.
        ///
        /// @return The page walk result.
        [[nodiscard]]
        PageWalk walk(const void *ptr);

        /// @brief Compact page tables, pruning empty tables.
        ///
        /// @warning This function can be very expensive and should be used sparingly.
        ///
        /// @return The number of pages freed.
        [[nodiscard]]
        size_t compact();

        /// @brief Get the physical address that backs a given virtual address.
        ///
        /// @param ptr The virtual address to query.
        ///
        /// @return The physical address that backs the given virtual address.
        [[nodiscard]]
        PhysicalAddress getBackingAddress(const void *ptr);

        /// @brief Get the memory flags for a given address.
        ///
        /// @param ptr The address to query.
        ///
        /// @return The memory flags for the given address.
        [[nodiscard]]
        PageFlags getMemoryFlags(const void *ptr);

        /// @brief Get the page size used to map a given address.
        ///
        /// @param ptr The address to query.
        ///
        /// @return The page size used to map the given address.
        [[nodiscard]]
        PageSize getPageSize(const void *ptr);

        /// @brief Map a range of memory to a virtual address.
        ///
        /// @note Equivalent to @c map(AddressMapping::of(range, vaddr), flags, type).
        ///
        /// @param range The range to map.
        /// @param vaddr The virtual address to map to.
        /// @param flags The flags to use for the mapping.
        /// @param type The type of memory to map.
        ///
        /// @retval OsStatusSuccess The mapping was successful, the full range is mapped.
        /// @retval OsStatusOutOfMemory There was not enough memory to map the range, no memory has been mapped.
        /// @retval OsStatusInvalidInput The address mapping was malformed, no memory has been mapped.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus map(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        [[nodiscard]]
        static OsStatus create(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, PageTables *tables [[gnu::nonnull]]) [[clang::allocating]];

#if __STDC_HOSTED__
        PageTableAllocator& TESTING_getPageTableAllocator() {
            return mAllocator;
        }
#endif
    };

    /// @brief Remap the kernel to replace the boot page tables.
    /// @param vmm The virtual memory manager
    /// @param paddr The physical address of the kernel
    /// @param vaddr The virtual address of the kernel
    void MapKernel(PageTables& vmm, PhysicalAddress paddr, const void *vaddr);

    /// @brief Reclaim bootloader memory.
    /// @param pm The page manager
    /// @param vmm The virtual memory manager
    void UpdateRootPageTable(const PageBuilder& pm, PageTables& vmm);
}
