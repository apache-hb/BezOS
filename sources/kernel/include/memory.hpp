#pragma once

#include "memory/page_allocator.hpp"
#include "memory/pte.hpp"
#include "memory/paging.hpp"
#include "memory/tables.hpp"

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
        SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, PageBuilder pm, AddressMapping pteMemory);

        void *allocate(size_t size, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        AddressMapping allocate(AllocateRequest request);

        void release(void *ptr, size_t size);

        void unmap(void *ptr, size_t size);
        void unmap(VirtualRange range) { ptes.unmap(range); }

        AddressMapping allocateStack(size_t size);

        MemoryRange pmmAllocate(size_t pages) {
            return pmm.alloc4k(pages);
        }

        void reserve(AddressMapping mapping) {
            reserveVirtual(mapping.virtualRange());
            reservePhysical(mapping.physicalRange());
        }

        void reservePhysical(MemoryRange range) {
            pmm.reserve(range);
        }

        void reserveVirtual(VirtualRange range) {
            ptes.reserve(range);
        }

        PageBuilder& getPager() { return pager; }

        PageTables& systemTables() { return ptes.ptes(); }
        SystemPageTables& pageTables() { return ptes; }
        PageAllocator& pmmAllocator() { return pmm; }

        void *map(MemoryRange range, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        template<typename T>
        T *mapObject(MemoryRange range, MemoryType type = MemoryType::eWriteBack) {
            return (T*)map(range, PageFlags::eData, type);
        }

        template<typename T>
        T *mapObject(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack) {
            return mapObject<T>(MemoryRange::of(paddr, sizeof(T)), type);
        }

        template<typename T>
        T *mmioRegion(PhysicalAddress begin) {
            return mapObject<T>(MemoryRange::of(begin, sizeof(T)), MemoryType::eUncached);
        }

        template<typename T>
        const T *mapConst(MemoryRange range, MemoryType type = MemoryType::eWriteBack) {
            return (const T*)map(range, PageFlags::eRead, type);
        }

        template<typename T>
        const T *mapConst(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack) {
            return mapConst<T>(MemoryRange::of(paddr, sizeof(T)), type);
        }
    };

    inline OsStatus AllocateMemory(PageAllocator& pmm, AddressSpaceAllocator *ptes, size_t pages, AddressMapping *mapping) {
        MemoryRange range = pmm.alloc4k(pages);
        if (range.isEmpty()) {
            return OsStatusOutOfMemory;
        }

        OsStatus status = ptes->map(range, PageFlags::eUserAll, MemoryType::eWriteBack, mapping);
        if (status != OsStatusSuccess) {
            pmm.release(range);
        }

        return status;
    }

    inline OsStatus AllocateMemory(PageAllocator& pmm, AddressSpaceAllocator *ptes, size_t pages, const void *hint, AddressMapping *mapping) {
        MemoryRange range = pmm.alloc4k(pages);
        if (range.isEmpty()) {
            return OsStatusOutOfMemory;
        }

        VirtualRange vmem = ptes->vmemAllocate({
            .size = range.size(),
            .align = x64::kPageSize,
            .hint = hint,
        });

        if (vmem.isEmpty()) {
            pmm.release(range);
            return OsStatusOutOfMemory;
        }

        AddressMapping map = MappingOf(range, vmem.front);

        if (OsStatus status = ptes->map(map, PageFlags::eUserAll)) {
            pmm.release(range);
            ptes->vmemRelease(vmem);
            return status;
        }

        *mapping = map;
        return OsStatusSuccess;
    }
}
