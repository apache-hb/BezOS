#pragma once

#include "memory/address_space.hpp"
#include "memory/allocator.hpp"
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

        MappingAllocation alloc(size_t size, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);
        void free(MappingAllocation allocation);

        void *allocate(size_t size, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        AddressMapping allocate(AllocateRequest request);

        OsStatus unmap(void *ptr, size_t size);
        OsStatus unmap(VirtualRange range) { return mTables.unmap(range); }
        OsStatus unmap(AddressMapping mapping);

        AddressMapping allocateStack(size_t size);

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

        PageBuilder& getPageManager() { return mPageManager; }
        PageTables& systemTables() { return *mTables.tables(); }
        AddressSpace& pageTables() { return mTables; }
        PageAllocator& pmmAllocator() { return mPageAllocator; }

        void *map(MemoryRange range, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);
        OsStatus map(size_t size, PageFlags flags, MemoryType type, AddressMapping *mapping);

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
}
