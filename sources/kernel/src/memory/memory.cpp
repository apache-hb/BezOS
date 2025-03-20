#include "memory/memory.hpp"

/// @brief The amount of memory a single pml4 entry can cover.
static constexpr auto kPml4MemorySize = x64::kHugePageSize * 512;

km::PageWalkIndices km::GetAddressParts(const void *ptr) noexcept [[clang::nonblocking]] {
    return GetAddressParts(reinterpret_cast<uintptr_t>(ptr));
}

km::AddressMapping km::detail::AlignLargeRangeHead(AddressMapping mapping) noexcept [[clang::nonblocking]] {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    uintptr_t aligned = sm::roundup(vaddr, x64::kLargePageSize);

    return AddressMapping {
        .vaddr = mapping.vaddr,
        .paddr = mapping.paddr,
        .size = aligned - vaddr,
    };
}

km::AddressMapping km::detail::AlignLargeRangeBody(AddressMapping mapping) noexcept [[clang::nonblocking]] {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    PhysicalAddress paddr = mapping.paddr;

    uintptr_t head2m = sm::roundup(vaddr, x64::kLargePageSize);
    uintptr_t tail2m = sm::rounddown(vaddr + mapping.size, x64::kLargePageSize);
    size_t offset = head2m - vaddr;
    size_t size = tail2m - head2m;

    return AddressMapping {
        .vaddr = reinterpret_cast<void*>(head2m),
        .paddr = paddr + offset,
        .size = size
    };
}

km::AddressMapping km::detail::AlignLargeRangeTail(AddressMapping mapping) noexcept [[clang::nonblocking]] {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    uintptr_t tail2m = sm::rounddown(vaddr + mapping.size, x64::kLargePageSize);
    size_t size = vaddr + mapping.size - tail2m;

    return AddressMapping {
        .vaddr = reinterpret_cast<void*>(tail2m),
        .paddr = mapping.paddr + mapping.size - size,
        .size = size
    };
}

bool km::detail::IsLargePageEligible(AddressMapping mapping) noexcept [[clang::nonblocking]] {
    //
    // Ranges smaller than 2m are not eligible for large pages.
    //
    if (mapping.size < x64::kLargePageSize) return false;

    uintptr_t paddr = mapping.paddr.address;
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);

    //
    // If the addresses are not aligned equally, the range is not eligible for large pages.
    // e.g. if the physical address is at 0x202000 and the virtual address is at 0x202000, the range is eligible.
    // But if the physical address is at 0x202000 and the virtual address is at 0x203000, the range is not eligible.
    //
    uintptr_t mask = x64::kLargePageSize - 1;
    if ((paddr & mask) != (vaddr & mask)) return false;

    //
    // After aligning the front of the range up to a 2m boundary and the size of the range down
    // to a 2m boundary, the range must still be larger than 2m to be eligible for large pages.
    //
    uintptr_t front2m = sm::roundup(paddr, x64::kLargePageSize);
    uintptr_t back2m = sm::rounddown(paddr + mapping.size, x64::kLargePageSize);

    return front2m < back2m;
}

size_t km::detail::GetCoveredSegments(VirtualRange range, size_t segment) noexcept [[clang::nonblocking]] {
    uintptr_t front = sm::rounddown(reinterpret_cast<uintptr_t>(range.front), segment);
    uintptr_t back = sm::roundup(reinterpret_cast<uintptr_t>(range.back), segment);

    return (back - front) / segment;
}

size_t km::detail::MaxPagesForMapping(VirtualRange range) noexcept [[clang::nonblocking]] {
    size_t count = 0;

    //
    // The range can cross over a pml4 boundary, in this case a simple modulo wont work so
    // we need to check which pml4 entries the range covers.
    //
    count += GetCoveredSegments(range, kPml4MemorySize);

    count += GetCoveredSegments(range, x64::kHugePageSize);

    //
    // If the range is smaller than 2m, we need to check if it crosses a page boundary
    // to know how many pages are required.
    //
    if (range.size() < x64::kLargePageSize) {
        uintptr_t front = sm::rounddown(reinterpret_cast<uintptr_t>(range.front), x64::kLargePageSize);
        uintptr_t back = sm::rounddown(reinterpret_cast<uintptr_t>(range.back), x64::kLargePageSize);

        if (front != back) {
            //
            // 2 pages for the pdte entries and 2 pages for the pte entries.
            //
            return count + 4;
        } else {
            //
            // We don't cross a boundary, so we only need 2 pages.
            //
            return count + 2;
        }
    }

    //
    // The range is larger than 2m, once again check if its aligned to a 2m boundary.
    // If it is aligned it will only require 1 page, otherwise it will require 2 pages.
    //

    if ((uintptr_t)range.front % x64::kLargePageSize != 0) {
        count += 1;
    }

    if ((uintptr_t)range.back % x64::kLargePageSize != 0) {
        count += 1;
    }

    count += GetCoveredSegments(range, x64::kLargePageSize);

    return count;
}
