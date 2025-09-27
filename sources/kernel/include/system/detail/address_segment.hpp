#pragma once

#include "memory/layout.hpp"

#include "memory/vmm_heap.hpp"
#include "std/detail/sticky_counter.hpp"

namespace sys::detail {
    /// @brief A segment of an address space.
    class AddressSegment {
        sm::detail::StickyCounter<size_t> mRefCount { 1 };
        km::MemoryRangeEx mBackingMemory;
        km::VmemAllocation mVmemAllocation;

    public:
        constexpr AddressSegment() noexcept = default;

        AddressSegment(km::MemoryRangeEx backingMemory, km::VmemAllocation vmemAllocation) noexcept;

        AddressSegment(const AddressSegment& other) noexcept;

        AddressSegment& operator=(const AddressSegment& other) noexcept;

        km::MemoryRangeEx getBackingMemory() const noexcept [[clang::nonallocating]];

        km::VmemAllocation getVmemAllocation() const noexcept [[clang::nonallocating]];

        km::AddressMapping mapping() const noexcept [[clang::nonallocating]];

        /// @brief Does this segment have physical memory backing it?
        ///
        /// If the segment has no backing memory, accessing it will result in a SIGBUS.
        /// This is distinct from a segment being mapped to a page with no access, which generates a SIGSEGV.
        ///
        /// @return True if the segment has backing memory, false otherwise.
        bool hasBackingMemory() const noexcept [[clang::nonblocking]];

        km::VirtualRange range() const noexcept [[clang::nonallocating]];
    };
}
