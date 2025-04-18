#pragma once

#include "memory/range.hpp"

#include "common/util/util.hpp"

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

        constexpr MemoryRange physicalRange() const {
            return { paddr, paddr + size };
        }

        VirtualRange virtualRange() const {
            return { vaddr, (const char*)vaddr + size };
        }

        constexpr intptr_t slide() const {
            return (intptr_t)vaddr - paddr.address;
        }

        constexpr bool isEmpty() const {
            return size == 0;
        }

        constexpr AddressMapping first(size_t bytes) const {
            return { vaddr, paddr, bytes };
        }

        AddressMapping last(size_t bytes) const {
            const char *vbegin = (const char*)vaddr + size - bytes;
            PhysicalAddress pbegin = paddr + size - bytes;
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
    };

    constexpr AddressMapping MappingOf(const void *vaddr, PhysicalAddress paddr, size_t size) {
        return AddressMapping { vaddr, paddr, size };
    }

    constexpr AddressMapping MappingOf(MemoryRange range, const void *vaddr) {
        return AddressMapping { vaddr, range.front, range.size() };
    }

    constexpr AddressMapping MappingOf(VirtualRange range, PhysicalAddress paddr) {
        return AddressMapping { range.front, paddr, range.size() };
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
    static void format(km::IOutStream& out, km::AddressMapping value) {
        out.format(value.virtualRange(), " -> ", value.physicalRange(), " (", sm::bytes(value.size), ")");
    }
};
