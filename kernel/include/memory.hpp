#pragma once

#include "memory/allocator.hpp"
#include "memory/paging.hpp"

namespace km {
    struct SystemMemory {
        PageManager pager;
        SystemMemoryLayout layout;
        PageAllocator pmm;
        VirtualAllocator vmm;

        SystemMemory(SystemMemoryLayout memory, uintptr_t bits, uintptr_t hhdmOffset, PageMemoryTypeLayout types);

        void *hhdmMap(PhysicalAddress begin, PhysicalAddress end, PageFlags flags = PageFlags::eData);

        template<typename T>
        T *hhdmMapObject(PhysicalAddress begin, PhysicalAddress end) {
            return (T*)hhdmMap(begin, end);
        }

        template<typename T>
        T *hhdmMapObject(PhysicalAddress paddr) {
            return hhdmMapObject<T>(paddr, paddr + sizeof(T));
        }
    };
}
