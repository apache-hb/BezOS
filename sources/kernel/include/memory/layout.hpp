#pragma once

#include "memory/range.hpp"

#include "util/util.hpp"
#include "util/memory.hpp"
#include "util/format.hpp"

#include "arch/paging.hpp"

#include <stddef.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace km {
    static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

    constexpr bool IsLowMemory(km::MemoryRange range) {
        return range.front < kLowMemory;
    }

    /// @brief Returns a memory range that is page aligned.
    ///
    /// @param range The memory range.
    /// @return The page aligned memory range.
    constexpr MemoryRange PageAligned(MemoryRange range) {
        return {sm::rounddown(range.front.address, x64::kPageSize), sm::roundup(range.back.address, x64::kPageSize)};
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

    struct AddressMapping {
        const void *vaddr;
        PhysicalAddress paddr;
        size_t size;

        MemoryRange physicalRange() const {
            return { paddr, paddr + size };
        }

        VirtualRange virtualRange() const {
            return { vaddr, (const char*)vaddr + size };
        }

        constexpr uintptr_t slide() const {
            return (uintptr_t)vaddr - paddr.address;
        }

        constexpr bool isEmpty() const {
            return size == 0;
        }
    };
}

template<>
struct km::Format<km::AddressMapping> {
    static void format(km::IOutStream& out, km::AddressMapping value) {
        out.format(value.virtualRange(), " -> ", value.physicalRange(), " (", sm::bytes(value.size), ")");
    }
};
