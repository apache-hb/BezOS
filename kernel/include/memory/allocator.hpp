#pragma once

#include "arch/paging.hpp"
#include "memory/layout.hpp"

#include "memory/page_allocator.hpp"
#include "memory/paging.hpp"
#include "memory/virtual_allocator.hpp"

namespace km {
    class PageTableManager {
        const km::PageBuilder *mPageManager;
        PageAllocator *mPageAllocator;
        VirtualAllocator *mVirtualAllocator;
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
        PageTableManager(const km::PageBuilder *pm, VirtualAllocator *vmm, PageAllocator *alloc);

        km::PhysicalAddress rootPageTable() const {
            return getPhysicalAddress(getRootTable());
        }

        void map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void mapRange(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void unmap(void *ptr, size_t size);

        km::PhysicalAddress getBackingAddress(const void *ptr) const;
    };
}

/// @brief Remap the kernel to replace the boot page tables.
/// @param pm The page manager
/// @param vmm The virtual memory manager
/// @param layout The system memory layout
/// @param paddr The physical address of the kernel
/// @param vaddr The virtual address of the kernel
void KmMapKernel(const km::PageBuilder& pm, km::PageTableManager& vmm, km::SystemMemoryLayout& layout, km::PhysicalAddress paddr, const void *vaddr);

/// @brief Migrate the memory range into hhdm.
/// @param vmm The virtual memory manager
/// @param pm The page manager
/// @param base The physical address
/// @param size The size of the memory range
/// @param type The memory type to use
void KmMigrateMemory(km::PageTableManager& vmm, km::PageBuilder& pm, km::PhysicalAddress base, size_t size, km::MemoryType type);

/// @brief Map a range of physical memory into virtual memory.
/// @param vmm The virtual memory manager
/// @param base The physical address
/// @param vaddr The virtual address
/// @param size The size of the memory range
/// @param flags The page flags
void KmMapMemory(km::PageTableManager& vmm, km::PhysicalAddress base, const void *vaddr, size_t size, km::PageFlags flags, km::MemoryType type);

/// @brief Reclaim bootloader memory.
/// @param pm The page manager
/// @param vmm The virtual memory manager
void KmReclaimBootMemory(const km::PageBuilder& pm, km::PageTableManager& vmm);
