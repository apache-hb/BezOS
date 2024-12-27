#pragma once

#include "memory/layout.hpp"

#include "memory/paging.hpp"

namespace km {
    enum class PageFlags {
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,
        eWriteThrough = 1 << 3,
        eCacheDisable = 1 << 4,

        eCode = eRead | eExecute,
        eData = eRead | eWrite,
        eAll = eRead | eWrite | eExecute,
    };

    UTIL_BITFLAGS(PageFlags);

    class PageAllocator {
        const SystemMemoryLayout *mLayout;
        int mCurrentRange;
        PhysicalAddress mOffset;

        MemoryRange currentRange() const;

        void setCurrentRange(int range);

    public:
        PageAllocator(const SystemMemoryLayout *layout);

        PhysicalPointer<x64::page> alloc4k();
    };

    class VirtualAllocator {
        const km::PageManager *mPageManager;
        PageAllocator *mPageAllocator;
        x64::page *mRootPageTable;

        x64::page *alloc4k();

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address);

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte);

        void mapRange4k(MemoryRange range, VirtualAddress vaddr, PageFlags flags);
        void mapRange2m(MemoryRange range, VirtualAddress vaddr, PageFlags flags);

    public:
        VirtualAllocator(const km::PageManager *pm, PageAllocator *alloc);

        x64::page *rootPageTable() {
            return mRootPageTable;
        }

        void map4k(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags);

        void map2m(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags);

        void mapRange(MemoryRange range, VirtualAddress vaddr, PageFlags flags);
    };
}

/// @brief Remap the kernel to replace the boot page tables.
/// @param pm The page manager
/// @param vmm The virtual memory manager
/// @param layout The system memory layout
/// @param paddr The physical address of the kernel
/// @param vaddr The virtual address of the kernel
void KmMapKernel(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout, km::PhysicalAddress paddr, km::VirtualAddress vaddr);

/// @brief Migrate the memory range into hhdm.
/// @param vmm The virtual memory manager
/// @param pm The page manager
/// @param base The virtual address
/// @param size The size of the memory range
void KmMigrateMemory(km::VirtualAllocator& vmm, km::PageManager& pm, const void *base, size_t size);

/// @brief Reclaim bootloader memory.
/// @param layout The system memory layout
void KmReclaimBootMemory(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout);
