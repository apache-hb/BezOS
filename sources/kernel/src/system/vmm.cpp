#include "system/vmm.hpp"

OsStatus sys2::AddressSpaceManager::map(MemoryManager *manager, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping) [[clang::allocating]] {

}

OsStatus sys2::AddressSpaceManager::unmap(MemoryManager *manager, km::AddressMapping mapping) [[clang::allocating]] {

}

OsStatus sys2::AddressSpaceManager::create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]] {
    km::TlsfHeap heap;
    km::MemoryRange range { (uintptr_t)vmem.front, (uintptr_t)vmem.back };
    if (OsStatus status = km::TlsfHeap::create(range, &heap)) {
        return status;
    }

    *manager = sys2::AddressSpaceManager(pm, pteMemory, flags, std::move(heap));
    return OsStatusSuccess;
}
