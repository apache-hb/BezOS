#pragma once

#include "memory/allocator.hpp"
#include "memory/paging.hpp"

#include <cstddef>

namespace km {
    struct SystemMemory {
        PageBuilder pager;
        PageAllocator pmm;
        PageTableManager vmm;

        SystemMemory(const boot::MemoryMap& memmap, PageBuilder pm, mem::IAllocator *allocator);

        void *allocate(
            size_t size,
            size_t align = alignof(std::max_align_t),
            PageFlags flags = PageFlags::eData,
            MemoryType type = MemoryType::eWriteBack
        );

        void release(void *ptr, size_t size);

        void unmap(void *ptr, size_t size);

        void *map(PhysicalAddress begin, PhysicalAddress end, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        template<typename T>
        T *mapObject(PhysicalAddress begin, PhysicalAddress end) {
            return (T*)map(begin, end);
        }

        template<typename T>
        T *mapObject(PhysicalAddress paddr) {
            return mapObject<T>(paddr, paddr + sizeof(T));
        }

        template<typename T>
        const T *mapConst(PhysicalAddress begin, PhysicalAddress end) {
            return (const T*)map(begin, end, PageFlags::eRead);
        }

        template<typename T>
        const T *mapConst(PhysicalAddress paddr) {
            return mapConst<T>(paddr, paddr + sizeof(T));
        }
    };
}
