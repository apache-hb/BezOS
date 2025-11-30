#pragma once

#include "std/std.hpp"

#include "common/physical_address.hpp"
#include "common/virtual_address.hpp"

#include "memory/layout.hpp"
#include "std/container/static_flat_map.hpp"

namespace km {
    struct MappingLookupTableStats {
        size_t entryCount;
        size_t maxEntries;
    };
}

namespace km::detail {
    /**
     * @brief Stores a lookup table of virtual to physical address mappings.
     *
     * Due to how the paging system is designed each @a km::PageTables will have its paging memory
     * provided by an external allocator, the "parent" allocator. On demand paging requires the page
     * table to be able to dynamically add and remove the "backing memory" where it stores its page tables.
     * This backing memory may not be contiguously placed in physical memory, and additionally may not be mapped
     * in a region of virtual memory which the PageTables owns. This class handles associating the physical
     * and virtual addresses for each segment as well as the reference count for each segment provided
     * by the parent allocator. The reference count is used so the PageTables knows when it is safe
     * to reclaim a segment of memory.
     *
     * This class has a couple of shortcomings, namely its lack of granularity and its fixed size.
     * As PageTables exist before any allocator infrastructure is available its not easily possible
     * to provide dynamic resizing of this structure. The lack of granularity is due to parent allocators
     * not always being able to support partial free'ing easily.
     */
    class MappingLookupTable {
        struct MapEntry {
            sm::VirtualAddress vaddr;
            uint32_t count;
            uint32_t refs;
        };

        using Cache = sm::StaticFlatMap<sm::PhysicalAddress, MapEntry>;

        Cache mCache;

    public:
        MappingLookupTable() noexcept [[clang::nonallocating]] = default;

        MappingLookupTable(void *storage [[gnu::nonnull]], size_t size) noexcept [[clang::nonallocating]]
            : mCache(storage, size)
        { }

        /**
         * @brief Track a new memory mapping.
         *
         * @param mapping The mapping to add.
         * @return The status of the operation.
         * @retval OsStatusSuccess The mapping was added successfully.
         * @retval OsStatusOutOfMemory There was not enough memory to track the mapping.
         */
        OsStatus addMemory(AddressMapping mapping) noexcept [[clang::nonallocating]];

        /**
         * @brief Attempts to reclaim an unused mapping.
         *
         * If any mapping is found with 0 references it is removed from the cache and returned.
         * This returns a single mapping at a time, the caller can call this in a loop until
         * @a OsStatusNotFound is returned.
         *
         * @param[out] mapping The reclaimed mapping, undefined if the return value is not @a OsStatusSuccess.
         *
         * @return The status of the operation.
         * @retval OsStatusSuccess A mapping was reclaimed successfully, and has been written to @p mapping.
         * @retval OsStatusNotFound There were no unused mappings to reclaim.
         */
        OsStatus reclaimMemory(AddressMapping *mapping [[outparam]]) noexcept [[clang::nonallocating]];

        /**
         * @brief Find the mapped virtual address for a given physical address.
         *
         * @param paddr The physical address to look up.
         * @return The virtual address mapped to the physical address, or @a VirtualAddress::invalid() if not found.
         */
        sm::VirtualAddress find(sm::PhysicalAddress paddr) const noexcept [[clang::nonblocking]];

        /**
         * @brief Retain a physical address, making sure it is not reclaimed.
         *
         * @param paddr The physical address to retain.
         */
        void retain(sm::PhysicalAddress paddr) noexcept [[clang::nonallocating]];

        /**
         * @brief Release a physical address.
         *
         * If an address reaches 0 references it can be reclaimed.
         *
         * @param paddr The physical address to release.
         */
        void release(sm::PhysicalAddress paddr) noexcept [[clang::nonallocating]];

        /**
         * @brief Collect statistics about the mapping lookup table.
         *
         * @return The collected statistics.
         */
        [[nodiscard]]
        MappingLookupTableStats stats() const noexcept [[clang::nonallocating]];
    };
}
