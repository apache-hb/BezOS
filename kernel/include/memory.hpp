#pragma once

#include "memory/allocator.hpp"
#include "memory/paging.hpp"

namespace km {
    struct SystemMemory {
        PageManager pager;
        SystemMemoryLayout layout;
        PageAllocator pmm;
        VirtualAllocator vmm;

        SystemMemory(SystemMemoryLayout memory, uintptr_t bits, limine_hhdm_response hhdm);

        VirtualAddress hhdmMap(PhysicalAddress paddr);
        VirtualAddress hhdmMap(PhysicalAddress begin, PhysicalAddress end);

        template<typename T>
        T *hhdmMapObject(PhysicalAddress begin, PhysicalAddress end) {
            VirtualAddress vaddr = hhdmMap(begin, end);
            return (T*)vaddr.address;
        }

        template<typename T>
        T *hhdmMapObject(PhysicalAddress paddr) {
            return hhdmMapObject<T>(paddr, paddr + sizeof(T));
        }
    };
}
