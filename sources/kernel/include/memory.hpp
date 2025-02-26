#pragma once

#include "memory/page_allocator.hpp"
#include "memory/pte.hpp"
#include "memory/paging.hpp"
#include "memory/tables.hpp"
#include "memory/virtual_allocator.hpp"

#include <cstddef>

namespace km {
    inline VirtualRange DefaultUserArea() {
        return VirtualRange { (void*)sm::megabytes(1).bytes(), (void*)sm::gigabytes(4).bytes() };
    }

    class SystemMemory {
        PageBuilder pager;
        PageAllocator pmm;
        SystemPageTables ptes;

    public:
        SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, VirtualRange userArea, PageBuilder pm, AddressMapping pteMemory);

        void *allocate(
            size_t size,
            PageFlags flags = PageFlags::eData,
            MemoryType type = MemoryType::eWriteBack
        );

        AddressMapping kernelAllocate(size_t size, PageFlags flags, MemoryType type);
        AddressMapping userAllocate(size_t size, PageFlags flags, MemoryType type);

        AddressMapping allocateWithHint(
            const void *hint,
            size_t size,
            PageFlags flags = PageFlags::eData,
            MemoryType type = MemoryType::eWriteBack
        );

        AddressMapping allocate(AllocateRequest request);

        void release(void *ptr, size_t size);

        void unmap(void *ptr, size_t size);

        AddressMapping allocateStack(size_t size);

        void reserve(AddressMapping mapping) {
            reserveVirtual(mapping.virtualRange());
            reservePhysical(mapping.physicalRange());
        }

        void reservePhysical(MemoryRange range) {
            pmm.markUsed(range);
        }

        void reserveVirtual(VirtualRange range) {
            ptes.reserve(range);
        }

        PageBuilder& getPager() { return pager; }

        PageTables& systemTables() { return ptes.ptes(); }

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
