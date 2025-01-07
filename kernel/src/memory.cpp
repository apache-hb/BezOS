#include "memory.hpp"

km::SystemMemory::SystemMemory(SystemMemoryLayout memory, PageBuilder pm)
    : pager(pm)
    , vmAllocator({ (void*)sm::megabytes(1).bytes(), (void*)pager.maxVirtualAddress() })
    , layout(memory)
    , pmm(&layout, pager.hhdmOffset())
    , vmm(&pager, &vmAllocator, &pmm)
{ }

// TODO: respect align, dont allocate such large memory ranges
void *km::SystemMemory::allocate(size_t size, size_t) {
    PhysicalAddress paddr = pmm.alloc4k(pages(size));
    KmDebugMessage("[INIT] Allocating ", Hex(size), " bytes at ", Hex(paddr.address), "\n");

    void *vaddr = (void*)(paddr.address + pager.hhdmOffset());
    MemoryRange range { paddr, paddr + size };
    vmm.mapRange(range, vaddr, PageFlags::eData, MemoryType::eWriteBack);
    return vaddr;
}

void km::SystemMemory::release(void *ptr, size_t size) {
    PhysicalAddress start = (uintptr_t)ptr - pager.hhdmOffset();
    MemoryRange range { start, start + size };
    vmm.unmap(ptr, size);
    pmm.release(range);
}

void km::SystemMemory::unmap(void *ptr, size_t size) {
    vmm.unmap(ptr, size);
}

void *km::SystemMemory::hhdmMap(PhysicalAddress begin, PhysicalAddress end, PageFlags flags, MemoryType type) {
    MemoryRange range { begin, end };

    void *vaddr = { (void*)(begin.address + pager.hhdmOffset()) };
    vmm.mapRange(range, vaddr, flags, type);
    pmm.markUsed(range);
    return vaddr;
}
