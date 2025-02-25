#pragma once

#include "allocator/allocator.hpp"
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
        mutable stdx::SpinLock mLock;
        const km::PageBuilder *mPageManager;
        mem::IAllocator *mAllocator;
        x64::PageMapLevel4 *mRootPageTable;

        x64::page *alloc4k();

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address);

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte);

        const x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        const x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        void mapRange4k(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);
        void mapRange2m(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);
        void mapRange1g(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);

        x64::PageMapLevel4 *getRootTable() const {
            return mRootPageTable;
        }

        template<typename T>
        T *getVirtualAddress(km::PhysicalAddress addr) const {
            return (T*)(addr.address + mPageManager->hhdmOffset());
        }

        km::PhysicalAddress getPhysicalAddress(const void *ptr) const {
            return km::PhysicalAddress { (uintptr_t)ptr - mPageManager->hhdmOffset() };
        }

    public:
        PageTableManager(const km::PageBuilder *pm, mem::IAllocator *allocator);

        km::PhysicalAddress rootPageTable() const {
            return getPhysicalAddress(getRootTable());
        }

        void map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void mapRange(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void unmap(void *ptr, size_t size);

        km::PhysicalAddress getBackingAddress(const void *ptr) const;
        PageFlags getMemoryFlags(const void *ptr) const;
        PageSize getPageSize(const void *ptr) const;

        void map(km::AddressMapping mapping, PageFlags flags, MemoryType type = MemoryType::eWriteBack) {
            mapRange(mapping.physicalRange(), mapping.vaddr, flags, type);
        }
    };
}

/// @brief Remap the kernel to replace the boot page tables.
/// @param vmm The virtual memory manager
/// @param paddr The physical address of the kernel
/// @param vaddr The virtual address of the kernel
void KmMapKernel(km::PageTableManager& vmm, km::PhysicalAddress paddr, const void *vaddr);

/// @brief Reclaim bootloader memory.
/// @param pm The page manager
/// @param vmm The virtual memory manager
void KmUpdateRootPageTable(const km::PageBuilder& pm, km::PageTableManager& vmm);
