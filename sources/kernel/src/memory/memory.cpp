#include "memory/memory.hpp"

km::PageWalkIndices km::GetAddressParts(const void *ptr) {
    return GetAddressParts(reinterpret_cast<uintptr_t>(ptr));
}

km::AddressMapping km::detail::AlignLargeRangeHead(AddressMapping mapping) {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    uintptr_t aligned = sm::roundup(vaddr, x64::kLargePageSize);

    return AddressMapping {
        .vaddr = mapping.vaddr,
        .paddr = mapping.paddr,
        .size = aligned - vaddr,
    };
}

km::AddressMapping km::detail::AlignLargeRangeBody(AddressMapping mapping) {
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

km::AddressMapping km::detail::AlignLargeRangeTail(AddressMapping mapping) {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    uintptr_t tail2m = sm::rounddown(vaddr + mapping.size, x64::kLargePageSize);
    size_t size = vaddr + mapping.size - tail2m;

    return AddressMapping {
        .vaddr = reinterpret_cast<void*>(tail2m),
        .paddr = mapping.paddr + mapping.size - size,
        .size = size
    };
}

bool km::detail::IsLargePageEligible(AddressMapping mapping) {
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
