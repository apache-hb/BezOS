#pragma once

#include "arch/paging.hpp"

#include "memory/memory.hpp"
#include "memory/paging.hpp"

namespace km {
    class PageTableManager {
        uintptr_t mSlide;
        mutable stdx::SpinLock mLock;
        const km::PageBuilder *mPageManager;
        mem::SynchronizedAllocator<mem::TlsfAllocator> mAllocator;
        x64::PageMapLevel4 *mRootPageTable;
        PageFlags mMiddleFlags;

        x64::page *alloc4k();

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

        const x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        const x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        OsStatus mapRange4k(AddressMapping mapping, PageFlags flags, MemoryType type);
        OsStatus mapRange2m(AddressMapping mapping, PageFlags flags, MemoryType type);
        OsStatus mapRange1g(AddressMapping mapping, PageFlags flags, MemoryType type);

        OsStatus map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);
        OsStatus map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);
        OsStatus map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        x64::PageMapLevel4 *getRootTable() const {
            return mRootPageTable;
        }

        template<typename T>
        T *asVirtual(km::PhysicalAddress addr) const {
            return (T*)(addr.address + mSlide);
        }

        km::PhysicalAddress asPhysical(const void *ptr) const;

    public:
        PageTableManager(const km::PageBuilder *pm, AddressMapping pteMemory, PageFlags middleFlags);

        km::PhysicalAddress root() const {
            return asPhysical(getRootTable());
        }

        OsStatus map(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);
        OsStatus unmap(VirtualRange range);
        OsStatus walk(const void *ptr, PageWalk *walk);

        km::PhysicalAddress getBackingAddress(const void *ptr);
        PageFlags getMemoryFlags(const void *ptr);
        PageSize2 getPageSize(const void *ptr);

        void unmap(void *ptr, size_t size) {
            unmap(VirtualRange { ptr, (char*)ptr + size });
        }

        OsStatus map(km::AddressMapping mapping, PageFlags flags, MemoryType type = MemoryType::eWriteBack) {
            return map(mapping.physicalRange(), mapping.vaddr, flags, type);
        }
    };

    /// @brief Remap the kernel to replace the boot page tables.
    /// @param vmm The virtual memory manager
    /// @param paddr The physical address of the kernel
    /// @param vaddr The virtual address of the kernel
    void MapKernel(km::PageTableManager& vmm, km::PhysicalAddress paddr, const void *vaddr);

    /// @brief Reclaim bootloader memory.
    /// @param pm The page manager
    /// @param vmm The virtual memory manager
    void UpdateRootPageTable(const km::PageBuilder& pm, km::PageTableManager& vmm);
}
