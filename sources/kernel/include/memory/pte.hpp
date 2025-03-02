#pragma once

#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"

#include "arch/paging.hpp"

#include "memory/memory.hpp"
#include "memory/paging.hpp"
#include "std/spinlock.hpp"

namespace km {
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
        PageFlags mMiddleFlags;

        /// @brief Allocate a new page table, garanteed to be aligned to 4k and zeroed.
        ///
        /// @return The new page table, or @c nullptr if no memory is available.
        x64::page *alloc4k();

        /// @brief Convert the physical address of a page table to a virtual address.
        ///
        /// All page tables are allocated from a memory pool which has a fixed offset.
        /// We can use that to convert between physical and virtual addresses.
        ///
        /// @param addr The physical address of the page table.
        /// @return The virtual address of the page table.
        template<typename T>
        T *asVirtual(PhysicalAddress addr) const {
            return (T*)(addr.address + mSlide);
        }

        /// @brief Convert the virtual address of a page table to a physical address.
        ///
        /// This can be used for storing a new page table in a pte.
        ///
        /// @param ptr The virtual address of the page table.
        /// @return The physical address of the page table.
        PhysicalAddress asPhysical(const void *ptr) const;

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

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address);

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte);

        x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        OsStatus mapRange4k(AddressMapping mapping, PageFlags flags, MemoryType type);
        OsStatus mapRange2m(AddressMapping mapping, PageFlags flags, MemoryType type);
        OsStatus mapRange1g(AddressMapping mapping, PageFlags flags, MemoryType type);

        OsStatus map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type);
        OsStatus map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type);
        OsStatus map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type);

        void partialRemap2m(x64::PageTable *pt, VirtualRange range, PhysicalAddress paddr, MemoryType type);

        /// @brief Split a 2m page mapping into 4k pages.
        ///
        /// @pre @p pde.is2m() must be true.
        /// @pre @p page.contains(erase) must be true.
        ///
        /// @param pde The 2m page directory entry to split.
        /// @param page The range of the 2m page.
        /// @param erase The range to unmap.
        /// @return The status of the operation.
        OsStatus split2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase);

        /// @brief Cut off one end of a 2m page mapping and replace it with 4k pages.
        ///
        /// @pre @p pde.is2m() must be true.
        /// @pre @p page.intersects(erase) must be true.
        ///
        /// @param pde The 2m page directory entry to split.
        /// @param page The range of the 2m page.
        /// @param erase The range to unmap.
        /// @return The status of the operation.
        OsStatus cut2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase);

        OsStatus unmap2mRegion(x64::pdte& pde, uintptr_t address, VirtualRange range);

    public:
        PageTables(const PageBuilder *pm, AddressMapping pteMemory, PageFlags middleFlags);

        const PageBuilder *pageManager() const { return mPageManager; }

        const x64::PageMapLevel4 *pml4() const { return mRootPageTable; }
        x64::PageMapLevel4 *pml4() { return mRootPageTable; }
        PhysicalAddress root() const { return asPhysical(pml4()); }

        OsStatus map(AddressMapping mapping, PageFlags flags, MemoryType type = MemoryType::eWriteBack);
        OsStatus unmap(VirtualRange range);
        OsStatus walk(const void *ptr, PageWalk *walk);

        PhysicalAddress getBackingAddress(const void *ptr);
        PageFlags getMemoryFlags(const void *ptr);
        PageSize getPageSize(const void *ptr);

        void unmap(void *ptr, size_t size) {
            unmap(VirtualRange { ptr, (char*)ptr + size });
        }

        OsStatus map(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);
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
