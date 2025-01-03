#pragma once

#include "memory/allocator.hpp"
#include "memory/paging.hpp"

#include <cstddef>

namespace km {
    struct SystemMemory {
        PageManager pager;
        SystemMemoryLayout layout;
        PageAllocator pmm;
        VirtualAllocator vmm;

        SystemMemory(SystemMemoryLayout memory, uintptr_t bits, uintptr_t hhdmOffset, PageMemoryTypeLayout types);

        void *allocate(size_t size, size_t align = alignof(std::max_align_t));

        void release(void *ptr, size_t size);

        void unmap(void *ptr, size_t size);

        void *hhdmMap(PhysicalAddress begin, PhysicalAddress end, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eUncachedOverridable);

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
