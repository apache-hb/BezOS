#include "memory/tables.hpp"

using namespace km;

SystemPageTables::SystemPageTables(AddressMapping pteMemory, const PageBuilder *pm, VirtualRange systemArea)
    : AddressSpaceAllocator(pteMemory, pm, PageFlags::eAll, PageFlags::eNone, systemArea)
{ }

ProcessPageTables::ProcessPageTables(SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea)
    : AddressSpaceAllocator(pteMemory, kernel->pageManager(), PageFlags::eUserAll, PageFlags::eUser, processArea)
    , mSystemTables(kernel)
{
    //
    // Copy the higher half mappings from the kernel ptes to the process ptes.
    //

    PageTables& system = mSystemTables->ptes();
    PageTables& process = ptes();

    x64::PageMapLevel4 *pml4 = system.pml4();
    x64::PageMapLevel4 *self = process.pml4();

    //
    // Only copy the higher half mappings, which are 256-511.
    //
    static constexpr size_t kCount = (sizeof(x64::PageMapLevel4) / sizeof(x64::pml4e)) / 2;
    std::copy_n(pml4->entries + kCount, kCount, self->entries + kCount);
}
