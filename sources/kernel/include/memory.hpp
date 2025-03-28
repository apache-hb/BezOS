#pragma once

#include "memory/address_space.hpp"
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
        PageBuilder mPageManager;
        PageAllocator mPageAllocator;
        AddressSpace mTables;

    public:
        SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, PageBuilder pm, AddressMapping pteMemory);

        void *allocate(size_t size, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        AddressMapping allocate(AllocateRequest request);

        OsStatus unmap(void *ptr, size_t size);
        OsStatus unmap(VirtualRange range) { return mTables.unmap(range); }

        AddressMapping allocateStack(size_t size);

        MemoryRange pmmAllocate(size_t pages) {
            return mPageAllocator.alloc4k(pages);
        }

        void reserve(AddressMapping mapping) {
            reserveVirtual(mapping.virtualRange());
            reservePhysical(mapping.physicalRange());
        }

        void reservePhysical(MemoryRange range) {
            mPageAllocator.reserve(range);
        }

        void reserveVirtual(VirtualRange range) {
            mTables.reserve(range);
        }

        PageBuilder& getPager() { return mPageManager; }

        [[deprecated]]
        PageTables& systemTables() { return *mTables.tables(); }

        AddressSpace& pageTables() { return mTables; }
        PageAllocator& pmmAllocator() { return mPageAllocator; }

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

    [[nodiscard]]
    inline OsStatus AllocateMemory(PageAllocator& pmm, AddressSpace& vmm, size_t pages, const void *hint, AddressMapping *mapping) {
        MemoryRange range = pmm.alloc4k(pages);
        if (range.isEmpty()) {
            return OsStatusOutOfMemory;
        }

        if (OsStatus status = vmm.map(range, hint, PageFlags::eUserAll, MemoryType::eWriteBack, mapping)) {
            pmm.release(range);
            return status;
        }

        return OsStatusSuccess;
    }

    [[nodiscard]]
    inline OsStatus AllocateMemory(PageAllocator& pmm, AddressSpace& vmm, size_t pages, AddressMapping *mapping) {
        return AllocateMemory(pmm, vmm, pages, nullptr, mapping);
    }
}
