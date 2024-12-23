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

    constexpr PageFlags operator|(PageFlags lhs, PageFlags rhs) {
        return static_cast<PageFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    constexpr PageFlags operator&(PageFlags lhs, PageFlags rhs) {
        return static_cast<PageFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

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

/// @brief remap the kernel to replace the boot page tables.
/// @param pm the page manager
/// @param vmm the virtual memory manager
/// @param layout the system memory layout
/// @param address the address of the kernel
void KmMapKernel(
    const km::PageManager& pm,
    km::VirtualAllocator& vmm,
    km::SystemMemoryLayout& layout,
    limine_kernel_address_response address
);
