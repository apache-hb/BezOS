#pragma once

#include "arch/paging.hpp"
#include "memory/layout.hpp"

#include "memory/paging.hpp"

namespace km {
    namespace detail {
        size_t GetRangeBitmapSize(MemoryRange range);
    }

    enum class PageFlags {
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,

        eCode = eRead | eExecute,
        eData = eRead | eWrite,
        eAll = eRead | eWrite | eExecute,
    };

    UTIL_BITFLAGS(PageFlags);

    class RegionBitmapAllocator {
        MemoryRange mRange;
        uint8_t *mBitmap;

        size_t bitCount() const;

        bool test(size_t bit) const;
        void set(size_t bit);
        void clear(size_t bit);

    public:
        /// @brief Construct a new region bitmap allocator.
        ///
        /// @pre The range must be page aligned.
        /// @pre The bitmap must be large enough to cover the range.
        ///
        /// @param range The range of memory to manage.
        /// @param bitmap The bitmap to use.
        RegionBitmapAllocator(MemoryRange range, uint8_t *bitmap);

        RegionBitmapAllocator() = default;

        PhysicalAddress alloc4k(size_t count);

        /// @brief Release a range of memory.
        ///
        /// @pre The range must have been allocated by this allocator.
        /// @pre The range must be page aligned.
        ///
        /// @param range The range to release.
        void release(MemoryRange range);

        /// @brief Check if the given address is within the range.
        ///
        /// @param addr The address to check.
        bool contains(PhysicalAddress addr) const {
            return mRange.contains(addr);
        }

        /// @brief Mark a range of memory as used.
        ///
        /// @param range The range to mark as used.
        void markAsUsed(MemoryRange range);
    };

    class PageAllocator {
        using RegionAllocators = stdx::StaticVector<RegionBitmapAllocator, SystemMemoryLayout::kMaxRanges>;

        /// @brief One allocator for each usable or reclaimable memory range.
        RegionAllocators mAllocators;

        /// @brief One allocator for each memory range below 1M.
        stdx::StaticVector<RegionBitmapAllocator, 4> mLowMemory;

    public:
        PageAllocator(const SystemMemoryLayout *layout, uintptr_t hhdmOffset);

        /// @brief Allocate a 4k page of memory above 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress alloc4k(size_t count = 1);

        void release(MemoryRange range);

        /// @brief Allocate a 4k page of memory below 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress lowMemoryAlloc4k();

        /// @brief Mark a range of memory as used.
        ///
        /// @param range The range to mark as used.
        void markRangeUsed(MemoryRange range);
    };

    class VirtualAllocator {
        const km::PageManager *mPageManager;
        PageAllocator *mPageAllocator;
        x64::page *mRootPageTable;

        x64::page *alloc4k();

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address);

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte);

        const x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        const x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        void mapRange4k(MemoryRange range, VirtualAddress vaddr, PageFlags flags, MemoryType type);
        void mapRange2m(MemoryRange range, VirtualAddress vaddr, PageFlags flags, MemoryType type);

    public:
        VirtualAllocator(const km::PageManager *pm, PageAllocator *alloc);

        x64::page *rootPageTable() {
            return mRootPageTable;
        }

        void map4k(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags, MemoryType type = MemoryType::eUncachedOverridable);

        void map2m(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags, MemoryType type = MemoryType::eUncachedOverridable);

        void mapRange(MemoryRange range, VirtualAddress vaddr, PageFlags flags, MemoryType type = MemoryType::eUncachedOverridable);

        void unmap(void *ptr, size_t size);
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
void KmMigrateMemory(km::VirtualAllocator& vmm, km::PageManager& pm, const void *base, size_t size, km::MemoryType type);

/// @brief Reclaim bootloader memory.
/// @param layout The system memory layout
void KmReclaimBootMemory(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout);
