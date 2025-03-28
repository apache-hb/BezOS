#include "memory/tables.hpp"

using namespace km;

AddressSpaceAllocator::AddressSpaceAllocator(AddressMapping pteMemory, const PageBuilder *pm, PageFlags flags, PageFlags extra, VirtualRange vmemArea)
    : mTables(pm, pteMemory, flags)
    , mExtraFlags(extra)
    , mVmemAllocator(vmemArea)
{ }

void AddressSpaceAllocator::reserve(VirtualRange range) {
    mVmemAllocator.reserve(range);
}

VirtualRange AddressSpaceAllocator::vmemAllocate(size_t pages) {
    size_t align = (pages > (x64::kLargePageSize / x64::kPageSize)) ? x64::kLargePageSize : x64::kPageSize;
    return mVmemAllocator.allocate({
        .size = pages * x64::kPageSize,
        .align = align,
    });
}

VirtualRange AddressSpaceAllocator::vmemAllocate(RangeAllocateRequest<const void*> request) {
    return mVmemAllocator.allocate({
        .size = request.size,
        .align = request.align,
        .hint = (const std::byte*)request.hint,
    });
}

void AddressSpaceAllocator::vmemRelease(VirtualRange range) {
    mVmemAllocator.release(range);
}

OsStatus AddressSpaceAllocator::map(AddressMapping mapping, PageFlags flags, MemoryType type) {
    return mTables.map(mapping, flags | mExtraFlags, type);
}

OsStatus AddressSpaceAllocator::map(MemoryRange range, PageFlags flags, MemoryType type, AddressMapping *mapping) {
    size_t pages = Pages(range.size());
    VirtualRange vmem = vmemAllocate(pages);
    if (vmem.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    AddressMapping m = MappingOf(vmem, range.front);
    OsStatus status = map(m, flags, type);
    if (status != OsStatusSuccess) {
        vmemRelease(vmem);
        return status;
    }

    *mapping = m;
    return OsStatusSuccess;
}

OsStatus AddressSpaceAllocator::unmap(VirtualRange range) {
    mTables.unmap(range);
    vmemRelease(range);
    return OsStatusSuccess;
}

void km::copyHigherHalfMappings(PageTables *tables, const PageTables *source) {
    const x64::PageMapLevel4 *pml4 = source->pml4();
    x64::PageMapLevel4 *self = tables->pml4();

    //
    // Higher half mappings are 256-511, This may not be enough as some of the higher
    // half mappings may not be used yet. In the future we will need to update process
    // page tables when the system pml4 is updated.
    //
    static constexpr size_t kCount = (sizeof(x64::PageMapLevel4) / sizeof(x64::pml4e)) / 2;
    std::copy_n(pml4->entries + kCount, kCount, self->entries + kCount);
}
