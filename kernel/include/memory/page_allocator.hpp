#pragma once

#include "boot.hpp"
#include "memory/range_allocator.hpp"

namespace km {
    enum class PageFlags {
        eNone = 0,
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,
        eUser = 1 << 3,

        eCode = eRead | eExecute,
        eData = eRead | eWrite,
        eAll = eRead | eWrite | eExecute,
        eAllUser = eRead | eWrite | eExecute | eUser,
    };

    UTIL_BITFLAGS(PageFlags);

    // TODO: this should be a wrapper around VirtualAllocator
    // and VirtualAllocator should be made generic to handle
    // both virtual and physical memory.
    class PageAllocator {
        /// @brief Allocator for memory below 1M.
        RangeAllocator<PhysicalAddress> mLowMemory;

        /// @brief Allocator for usable memory above 1M.
        RangeAllocator<PhysicalAddress> mMemory;

    public:
        PageAllocator(const boot::MemoryMap& memmap, mem::IAllocator *allocator);

        /// @brief Allocate a 4k page of memory above 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress alloc4k(size_t count = 1);

        /// @brief Release a range of memory.
        ///
        /// @param range The range to release.
        void release(MemoryRange range);

        /// @brief Allocate a 4k page of memory below 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress lowMemoryAlloc4k();

        /// @brief Mark a range of memory as used.
        ///
        /// @param range The range to mark as used.
        void markUsed(MemoryRange range);
    };
}
