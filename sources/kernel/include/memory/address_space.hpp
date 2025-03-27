#pragma once

#include "memory/paging.hpp"
#include "memory/pte.hpp"
#include "memory/range.hpp"
#include "memory/tables.hpp"
#include <type_traits>

namespace km {
    class AddressSpace {
        PageTables mTables;
        VmemAllocator mVmemAllocator;

    public:
        AddressSpace(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, VirtualRange vmem);

        [[nodiscard]]
        OsStatus unmap(VirtualRange range);

        [[nodiscard]]
        OsStatus unmap(void *ptr, size_t size);

        [[nodiscard]]
        void *map(MemoryRange range, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapObject(MemoryRange range, MemoryType type = MemoryType::eWriteBack);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapObject(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(MemoryRange range, MemoryType type = MemoryType::eWriteBack);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(PhysicalAddress paddr, PageFlags flags = PageFlags::eData);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(MemoryRange range, PageFlags flags = PageFlags::eData);
    };
}
