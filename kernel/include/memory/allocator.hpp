#pragma once

#include "memory/layout.hpp"

#include "memory/paging.hpp"

namespace km {
    enum class PageFlags {
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,

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

    public:
        VirtualAllocator(const km::PageManager *pm, PageAllocator *alloc);

        x64::page *rootPageTable() {
            return mRootPageTable;
        }

        void map4k(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags);
    };
}

/// @brief Remap the kernel to replace the boot page tables.
/// @param pm The page manager
/// @param vmm The virtual memory manager
/// @param layout The system memory layout
/// @param address The address of the kernel
void KmMapKernel(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout, limine_kernel_address_response address);

/// @brief Migrate the kernel stack out of bootloader reclaimable memory.
/// @param vmm The virtual memory manager
/// @param pm The page manager
/// @param base The old stack address
/// @param size The old stack size
void KmMigrateStack(km::VirtualAllocator& vmm, km::PageManager& pm, const void *base, size_t size);

/// @brief Reclaim bootloader memory.
/// @param layout The system memory layout
void KmReclaimBootMemory(km::SystemMemoryLayout& layout);
