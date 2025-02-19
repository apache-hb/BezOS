#pragma once

#include "memory/allocator.hpp"
#include "memory/paging.hpp"
#include "memory/virtual_allocator.hpp"

#include <cstddef>

namespace km {
    struct SystemMemory {
        mem::IAllocator *allocator;
        PageBuilder pager;
        PageAllocator pmm;
        PageTableManager pt;
        VirtualAllocator vmm;

        SystemMemory(const boot::MemoryMap& memmap, VirtualRange systemArea, PageBuilder pm, mem::IAllocator *allocator);

        void *allocate(
            size_t size,
            size_t align = alignof(std::max_align_t),
            PageFlags flags = PageFlags::eData,
            MemoryType type = MemoryType::eWriteBack
        );

        AddressMapping userAllocate(size_t size, PageFlags flags, MemoryType type);

        void release(void *ptr, size_t size);

        void unmap(void *ptr, size_t size);

        void *map(PhysicalAddress begin, PhysicalAddress end, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        void *map(MemoryRange range, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack) {
            return map(range.front, range.back, flags, type);
        }

        template<typename T>
        T *mapObject(PhysicalAddress begin, PhysicalAddress end, MemoryType type = MemoryType::eWriteBack) {
            return (T*)map(begin, end, PageFlags::eData, type);
        }

        template<typename T>
        T *mapObject(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack) {
            return mapObject<T>(paddr, paddr + sizeof(T), type);
        }

        template<typename T>
        T *mmioRegion(PhysicalAddress begin) {
            return mapObject<T>(begin, begin + sizeof(T), MemoryType::eUncached);
        }

        template<typename T>
        const T *mapConst(PhysicalAddress begin, PhysicalAddress end, MemoryType type = MemoryType::eWriteBack) {
            return (const T*)map(begin, end, PageFlags::eRead, type);
        }

        template<typename T>
        const T *mapConst(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack) {
            return mapConst<T>(paddr, paddr + sizeof(T), type);
        }
    };
}
