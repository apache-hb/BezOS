#pragma once

#include "memory/layout.hpp"
#include "memory/pmm_heap.hpp"
#include "memory/vmm_heap.hpp"

namespace km {
    class StackMappingAllocation {
        km::PmmAllocation mMemoryAllocation;
        km::VmemAllocation mAddressAllocation;
        km::VmemAllocation mGuardHead;
        km::VmemAllocation mGuardTail;

        StackMappingAllocation(km::PmmAllocation memory, km::VmemAllocation address, km::VmemAllocation head, km::VmemAllocation tail) noexcept;
    public:
        constexpr StackMappingAllocation() noexcept = default;

        km::StackMapping stackMapping() const noexcept [[clang::nonblocking]];

        km::PhysicalAddressEx baseMemory() const noexcept [[clang::nonblocking]];

        km::MemoryRangeEx memory() const noexcept [[clang::nonblocking]];

        km::VirtualRangeEx stack() const noexcept [[clang::nonblocking]];

        sm::VirtualAddress baseAddress() const noexcept [[clang::nonblocking]];

        sm::VirtualAddress stackBaseAddress() const noexcept [[clang::nonblocking]];

        size_t stackSize() const noexcept [[clang::nonblocking]];

        size_t totalSize() const noexcept [[clang::nonblocking]];

        km::VmemAllocation guardHead() const noexcept [[clang::nonblocking]];

        bool hasGuardHead() const noexcept [[clang::nonblocking]];

        km::VmemAllocation guardTail() const noexcept [[clang::nonblocking]];

        bool hasGuardTail() const noexcept [[clang::nonblocking]];

        km::PmmAllocation memoryAllocation() const noexcept [[clang::nonblocking]];

        km::VmemAllocation stackAllocation() const noexcept [[clang::nonblocking]];

        [[nodiscard]]
        static OsStatus create(PmmAllocation memory, VmemAllocation address, VmemAllocation head, VmemAllocation tail, StackMappingAllocation *result [[outparam]]);

        static StackMappingAllocation unchecked(PmmAllocation memory, VmemAllocation address, VmemAllocation head, VmemAllocation tail);
    };
}
