#pragma once

#include "memory/range.hpp"

#include "common/util/util.hpp"

#include "panic.hpp"
#include "util/memory.hpp"
#include "util/format.hpp"

#include "arch/paging.hpp"

#include <stddef.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace km {
    static constexpr size_t kLowMemory = sm::megabytes(1).bytes();
    static constexpr size_t k32BitMemory = sm::gigabytes(4).bytes();

    constexpr bool IsLowMemory(MemoryRange range) {
        return range.front < kLowMemory;
    }

    /// @brief Returns a memory range that is page aligned.
    ///
    /// @param range The memory range.
    /// @return The page aligned memory range.
    constexpr MemoryRange PageAligned(MemoryRange range) {
        return alignedOut(range, x64::kPageSize);
    }

    /// @brief Returns the number of pages required to store the given number of bytes.
    ///
    /// @param bytes The number of bytes.
    /// @return The number of pages.
    constexpr size_t Pages(size_t bytes) {
        return sm::roundup(bytes, x64::kPageSize) / x64::kPageSize;
    }

    constexpr size_t LargePages(size_t bytes) {
        return sm::roundup(bytes, x64::kLargePageSize) / x64::kLargePageSize;
    }

    /// @brief A mapping of a virtual address to a physical address.
    struct AddressMapping {
        const void *vaddr;
        PhysicalAddress paddr;
        size_t size;

        sm::VirtualAddress virtualAddress() const {
            return vaddr;
        }

        sm::PhysicalAddress physicalAddress() const {
            return paddr.address;
        }

        constexpr MemoryRange physicalRange() const {
            return { paddr, paddr + size };
        }

        constexpr MemoryRangeEx physicalRangeEx() const {
            return { paddr.address, paddr.address + size };
        }

        VirtualRange virtualRange() const {
            return { vaddr, (const char*)vaddr + size };
        }

        VirtualRangeEx virtualRangeEx() const {
            return { (uintptr_t)vaddr, (uintptr_t)vaddr + size };
        }

        constexpr uintptr_t slide() const {
            return (uintptr_t)vaddr - paddr.address;
        }

        constexpr bool isEmpty() const {
            return size == 0;
        }

        /// @brief Create a mapping that contains the first N bytes of the mapping.
        constexpr AddressMapping first(size_t bytes) const {
            KM_ASSERT(bytes <= size);

            return { vaddr, paddr, bytes };
        }

        /// @brief Create a mapping that contains the last N bytes of the mapping.
        AddressMapping last(size_t bytes) const {
            KM_ASSERT(bytes <= size);

            const char *vbegin = (const char*)vaddr + size - bytes;
            PhysicalAddress pbegin = paddr + size - bytes;
            return { vbegin, pbegin, bytes };
        }

        /// @brief Create a mapping that contains the specified subrange.
        constexpr AddressMapping subrange(size_t offset, size_t bytes) const {
            KM_ASSERT(offset + bytes <= size);

            const void *vbegin = (const char*)vaddr + offset;
            PhysicalAddress pbegin = paddr + offset;
            return { vbegin, pbegin, bytes };
        }

        constexpr AddressMapping subrange(size_t offset) const {
            KM_ASSERT(offset < size);

            return subrange(offset, size - offset);
        }

        /// @brief Create a mapping that contains the subset of this mapping and the given range.
        constexpr AddressMapping subrange(VirtualRange range) const {
            VirtualRange subset = intersection(virtualRange(), range);
            if (subset.isEmpty()) {
                return AddressMapping {};
            }

            size_t offset = (const char*)subset.front - (const char*)vaddr;
            size_t bytes = (const char*)subset.back - (const char*)subset.front;
            const void *vbegin = (const char*)vaddr + offset;
            PhysicalAddress pbegin = paddr + offset;
            return { vbegin, pbegin, bytes };
        }

        constexpr AddressMapping offset(intptr_t offset) const {
            return { (const char*)vaddr + offset, paddr + offset, size };
        }

        /// @brief Return a new mapping that is aligned to the given size.
        ///
        /// The new mapping is garunteed to be aligned to the given size and will
        /// cover at least the same range as the original mapping.
        ///
        /// @pre @e equalOffsetTo(align) must be true.
        ///
        /// @param align The alignment size.
        /// @return The aligned mapping.
        constexpr AddressMapping aligned(size_t align) const {
            PhysicalAddress alignedHead = sm::rounddown(paddr.address, align);
            size_t alignedTail = sm::roundup(size + paddr.address, align);
            size_t alignedSize = (alignedTail - alignedHead.address);

            const void *alignedVaddr = (void*)sm::rounddown(uintptr_t(vaddr), align);
            return { alignedVaddr, alignedHead, alignedSize };
        }

        /// @brief Are the virtual and physical addresses aligned to the given size.
        constexpr bool alignsExactlyTo(size_t align) const {
            return (uintptr_t)vaddr % align == 0 && paddr.address % align == 0;
        }

        /// @brief Do the virtual and physical addresses have the same alignment relative to the given size.
        constexpr bool equalOffsetTo(size_t align) const {
            return ((uintptr_t)vaddr % align) == (paddr.address % align);
        }

        constexpr bool operator==(const AddressMapping& other) const noexcept = default;
        constexpr bool operator!=(const AddressMapping& other) const noexcept = default;
    };

    constexpr AddressMapping MappingOf(const void *vaddr, PhysicalAddress paddr, size_t size) {
        return AddressMapping { vaddr, paddr, size };
    }

    constexpr AddressMapping MappingOf(MemoryRange range, const void *vaddr) {
        return MappingOf(vaddr, range.front, range.size());
    }

    constexpr AddressMapping MappingOf(VirtualRange range, PhysicalAddress paddr) {
        return MappingOf(range.front, paddr, range.size());
    }

    constexpr AddressMapping MappingOf(const void *vaddr, PhysicalAddressEx paddr, size_t size) {
        return MappingOf(vaddr, std::bit_cast<PhysicalAddress>(paddr), size);
    }

    constexpr AddressMapping MappingOf(MemoryRangeEx range, const void *vaddr) {
        return MappingOf(vaddr, std::bit_cast<PhysicalAddress>(range.front), range.size());
    }

    constexpr AddressMapping MappingOf(VirtualRangeEx range, PhysicalAddressEx paddr) {
        return MappingOf(range.front, paddr, range.size());
    }

    struct StackMapping {
        /// @brief The mapped area of stack.
        AddressMapping stack;

        /// @brief The total mapped area including guard pages.
        VirtualRange total;

        void *baseAddress() const {
            return (void*)((uintptr_t)stack.vaddr + stack.size);
        }
    };
}

template<>
struct km::Format<km::AddressMapping> {
    static constexpr size_t kStringSize
        = kFormatSize<km::VirtualRange> + 4
        + kFormatSize<km::MemoryRange> + 2
        + kFormatSize<sm::Memory> + 1;

    static void format(km::IOutStream& out, km::AddressMapping value) {
        out.format(value.virtualRange(), " -> ", value.physicalRange(), " (", sm::bytes(value.size), ")");
    }
};
