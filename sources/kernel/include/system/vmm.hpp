#pragma once

#include "memory/heap.hpp"
#include "memory/layout.hpp"
#include "memory/memory.hpp"
#include "memory/paging.hpp"
#include "memory/page_tables.hpp"

#include "memory/vmm_heap.hpp"
#include "std/detail/sticky_counter.hpp"
#include "std/spinlock.hpp"
#include "system/detail/range_table.hpp"

#include "common/compiler/compiler.hpp"

namespace km {
    class AddressSpace;
}

namespace sys {
    class MemoryManager;

    class AddressSegment {
        sm::detail::StickyCounter<size_t> mRefCount { 1 };
        km::MemoryRange mBackingMemory;
        km::VmemAllocation mVmemAllocation;

    public:
        constexpr AddressSegment() noexcept = default;

        constexpr AddressSegment(km::MemoryRange backingMemory, km::VmemAllocation vmemAllocation) noexcept
            : mBackingMemory(backingMemory)
            , mVmemAllocation(vmemAllocation)
        {
            KM_ASSERT(!vmemAllocation.isNull());
        }

        constexpr AddressSegment(const AddressSegment& other) noexcept
            : AddressSegment(other.getBackingMemory(), other.getVmemAllocation())
        { }

        constexpr AddressSegment& operator=(const AddressSegment& other) noexcept {
            if (this != &other) {
                mBackingMemory = other.getBackingMemory();
                mVmemAllocation = other.getVmemAllocation();
            }

            return *this;
        }

        km::MemoryRange getBackingMemory() const noexcept [[clang::nonallocating]] {
            return mBackingMemory;
        }

        km::VmemAllocation getVmemAllocation() const noexcept [[clang::nonallocating]] {
            return mVmemAllocation;
        }

        km::AddressMapping mapping() const noexcept [[clang::nonallocating]] {
            auto front = mVmemAllocation.address();
            const void *vaddr = std::bit_cast<const void*>(front);
            return km::MappingOf(mBackingMemory, vaddr);
        }

        /// @brief Does this segment have physical memory backing it?
        ///
        /// If the segment has no backing memory, accessing it will result in a SIGBUS.
        /// This is distinct from a segment being mapped to a page with no access, which generates a SIGSEGV.
        ///
        /// @return True if the segment has backing memory, false otherwise.
        bool hasBackingMemory() const noexcept [[clang::nonblocking]] {
            return !mBackingMemory.isEmpty();
        }

        km::VirtualRange range() const noexcept [[clang::nonallocating]] {
            return mVmemAllocation.range().cast<const void*>();
        }
    };

    struct AddressSpaceManagerStats {
        km::TlsfHeapStats heapStats;
        size_t segments;

        constexpr size_t controlMemory() const noexcept [[clang::nonallocating]] {
            return segments * sizeof(AddressSegment);
        }
    };

    class AddressSpaceManager {
        using Table = sys::detail::RangeTable<AddressSegment>;
        using Map = typename Table::Map;
        using Iterator = typename Map::iterator;

        stdx::SpinLock mLock;

        Table mTable GUARDED_BY(mLock);
        km::AddressMapping mPteMemory;
        km::PageTables mPageTables GUARDED_BY(mLock);
        km::VmemHeap mHeap GUARDED_BY(mLock);

        Map& segments() noexcept REQUIRES(mLock) {
            return mTable.segments();
        }

        AddressSpaceManager(km::AddressMapping pteMemory, km::PageTables&& pt, km::VmemHeap &&heap) noexcept
            : mPteMemory(pteMemory)
            , mPageTables(std::move(pt))
            , mHeap(std::move(heap))
        { }

        enum ReleaseSide {
            kLow,
            kHigh,
        };

        void deleteSegment(MemoryManager *manager, AddressSegment&& segment) noexcept [[clang::allocating]] REQUIRES(mLock);

        Iterator eraseSegment(MemoryManager *manager, Iterator it) noexcept [[clang::allocating]] REQUIRES(mLock);
        void eraseSegment(MemoryManager *manager, km::VirtualRange segment) noexcept [[clang::allocating]] REQUIRES(mLock);
        void eraseMany(MemoryManager *manager, km::VirtualRange range) noexcept [[clang::allocating]] REQUIRES(mLock);

        void addSegment(AddressSegment &&segment) noexcept [[clang::allocating]] REQUIRES(mLock);
        void addNoAccessSegment(km::VmemAllocation allocation) noexcept [[clang::allocating]] REQUIRES(mLock);

        /// @param manager The memory manager to use.
        /// @param it The iterator to the segment to map.
        /// @param src The full source range that is being mapped.
        /// @param dst The destination range to map to.
        /// @param flags The page flags to use.
        /// @param type The memory type to use.
        OsStatus mapSegment(
            MemoryManager *manager,
            Iterator it,
            km::VirtualRange src,
            km::VirtualRange dst,
            km::PageFlags flags,
            km::MemoryType type,
            km::VmemAllocation allocation
        ) [[clang::allocating]] REQUIRES(mLock);

        OsStatus splitSegment(MemoryManager *manager, Iterator it, const void *midpoint, ReleaseSide side) [[clang::allocating]] REQUIRES(mLock);
        OsStatus unmapSegment(MemoryManager *manager, Iterator it, km::VirtualRange range, km::VirtualRange *remaining) [[clang::allocating]] REQUIRES(mLock);

        OsStatus splitAtAddress(MemoryManager *manager, sm::VirtualAddress address) [[clang::allocating]] REQUIRES(mLock);
    public:
        UTIL_NOCOPY(AddressSpaceManager);

        constexpr AddressSpaceManager(AddressSpaceManager&& other) noexcept
            : mTable(std::move(other.mTable))
            , mPteMemory(std::move(other.mPteMemory))
            , mPageTables(std::move(other.mPageTables))
            , mHeap(std::move(other.mHeap))
        { }

        constexpr AddressSpaceManager& operator=(AddressSpaceManager&& other) noexcept {
            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

            mTable = std::move(other.mTable);
            mPteMemory = std::move(other.mPteMemory);
            mPageTables = std::move(other.mPageTables);
            mHeap = std::move(other.mHeap);
            return *this;

            CLANG_DIAGNOSTIC_POP();
        }

        constexpr AddressSpaceManager() noexcept = default;

        km::PageTables& getPageTables() noexcept {
            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety"); // TODO: ouch

            return mPageTables;

            CLANG_DIAGNOSTIC_POP();
        }

        /// @brief Allocate new memory in the current address space.
        [[nodiscard]]
        OsStatus map(MemoryManager *manager, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]];

        /// @brief Map a segment of memory into this address space, allocating virtual address space for it.
        [[nodiscard]]
        OsStatus map(MemoryManager *manager, km::MemoryRange range, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]];

        /// @brief Map memory from another address space into newly allocated space in this address space.
        [[nodiscard]]
        OsStatus map(MemoryManager *manager, AddressSpaceManager *other, km::VirtualRange range, km::PageFlags flags, km::MemoryType type, km::VirtualRange *result [[outparam]]) [[clang::allocating]];

        /// @brief Map physical memory into this address space at a fixed address.
        [[nodiscard]]
        OsStatus map(MemoryManager *manager, sm::VirtualAddress address, km::MemoryRange memory, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]];

        /// @brief Map physical memory that is not managed by the system memory manager.
        [[nodiscard]]
        OsStatus mapExternal(km::MemoryRange memory, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]];

        /// @brief Allocate virtual memory in this address space and back it with physical memory from the memory manager.
        ///
        /// @param memory The range of physical memory to back the allocation with.
        /// @param address The address to allocate at, or null to allocate anywhere.
        /// @param size The size of the allocation.
        /// @param align The alignment of the allocation.
        /// @param addressIsHint If true, the address is a hint and may be ignored.
        /// @param flags The page flags to use for the mapping.
        /// @param[out] result The resulting mapping.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus allocateVirtual(km::MemoryRange memory, sm::VirtualAddress address, size_t size, size_t align, bool addressIsHint, km::PageFlags flags, km::AddressMapping *result [[outparam]]) [[clang::allocating]];

        [[nodiscard]]
        OsStatus unmap(MemoryManager *manager, km::VirtualRange range) [[clang::allocating]];

        [[nodiscard]]
        OsStatus querySegment(const void *address, AddressSegment *result) noexcept [[clang::nonallocating]];

        AddressSpaceManagerStats stats() noexcept [[clang::nonallocating]];

        void destroy(MemoryManager *manager) [[clang::allocating]];

        km::AddressMapping getPteMemory() const noexcept [[clang::nonallocating]] {
            return mPteMemory;
        }

        km::PhysicalAddress getPageMap() const noexcept [[clang::nonallocating]];

        void dump() noexcept;

        [[nodiscard]]
        static OsStatus create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager [[outparam]]) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(const km::AddressSpace *pt, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager [[outparam]]) [[clang::allocating]];

        static void setActiveMap(const AddressSpaceManager *map) noexcept;
    };
}
