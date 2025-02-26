#pragma once

#include "arch/paging.hpp"

#include "memory/memory.hpp"
#include "memory/paging.hpp"

namespace km {
    enum class PageSize {
        eNone = 0,
        eDefault = x64::kPageSize,
        eLarge = x64::kLargePageSize,
        eHuge = x64::kHugePageSize,
    };

    class PageTableManager {
        uintptr_t mSlide;
        mutable stdx::SpinLock mLock;
        const km::PageBuilder *mPageManager;
        mem::SynchronizedAllocator<mem::TlsfAllocator> mAllocator;
        x64::PageMapLevel4 *mRootPageTable;

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

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, PageFlags flags);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, PageFlags flags);

        const x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        const x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        void mapRange4k(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);
        void mapRange2m(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);
        void mapRange1g(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);

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
        PageTableManager(const km::PageBuilder *pm, AddressMapping pteMemory);

        km::PhysicalAddress root() const {
            return asPhysical(getRootTable());
        }

        void map(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void unmap(void *ptr, size_t size);

        km::PhysicalAddress getBackingAddress(const void *ptr) const;
        PageFlags getMemoryFlags(const void *ptr);
        PageSize2 getPageSize(const void *ptr);

        void map(km::AddressMapping mapping, PageFlags flags, MemoryType type = MemoryType::eWriteBack) {
            map(mapping.physicalRange(), mapping.vaddr, flags, type);
        }

        OsStatus walk(const void *ptr, PageWalk *walk);
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
