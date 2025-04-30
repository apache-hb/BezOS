#pragma once

#include "memory/address_space.hpp"
#include "memory/page_allocator.hpp"
#include "memory/pte.hpp"
#include "memory/paging.hpp"
#include "memory/tables.hpp"

#include <cstddef>

namespace km {
    class MappingAllocation;
    class StackMappingAllocation;

    inline VirtualRange DefaultUserArea() {
        return VirtualRange { (void*)sm::megabytes(1).bytes(), (void*)sm::gigabytes(4).bytes() };
    }

    class SystemMemory {
        PageBuilder mPageManager;
        PageAllocator mPageAllocator;
        AddressSpace mTables;

    public:
        constexpr SystemMemory() noexcept = default;

        SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, PageBuilder pm, AddressMapping pteMemory);

        void *allocate(size_t size, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        AddressMapping allocate(AllocateRequest request);

        OsStatus unmap(void *ptr, size_t size);
        OsStatus unmap(VirtualRange range);

        AddressMapping allocateStack(size_t size);

        void reserve(AddressMapping mapping);

        void reservePhysical(MemoryRange range);

        void reserveVirtual(VirtualRange range);

        PageBuilder& getPageManager() { return mPageManager; }
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

        [[nodiscard]]
        OsStatus mapStack(size_t size, PageFlags flags, StackMappingAllocation *mapping [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus map(size_t size, PageFlags flags, MemoryType type, MappingAllocation *allocation [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus unmap(MappingAllocation allocation);

        [[nodiscard]]
        static OsStatus create(std::span<const boot::MemoryRegion> memmap, VirtualRangeEx systemArea, PageBuilder pm, AddressMapping pteMemory, SystemMemory *system [[gnu::nonnull]]);
    };
}
