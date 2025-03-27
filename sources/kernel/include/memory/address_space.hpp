#pragma once

#include "memory/paging.hpp"
#include "memory/pte.hpp"
#include "memory/range.hpp"
#include "memory/virtual_allocator.hpp"
#include <type_traits>

namespace km {
    namespace detail {
        template<typename T>
        static constexpr PageFlags kDefaultPageFlags = std::is_const_v<T> ? PageFlags::eRead : PageFlags::eData;
    }

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
        T *mapObject(MemoryRange range, PageFlags flags = detail::kDefaultPageFlags<T>, MemoryType type = MemoryType::eWriteBack) {
            return (T*)map(range, flags, type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapObject(PhysicalAddress paddr, PageFlags flags = detail::kDefaultPageFlags<T>, MemoryType type = MemoryType::eWriteBack) {
            return mapObject<T>(MemoryRange::of(paddr, sizeof(T)), flags, type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(MemoryRange range, MemoryType type = MemoryType::eWriteBack) {
            return (const T*)map(range, PageFlags::eRead, type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack) {
            return mapConst<T>(MemoryRange::of(paddr, sizeof(T)), type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(PhysicalAddress paddr, PageFlags flags = detail::kDefaultPageFlags<T>) {
            return mapObject<T>(MemoryRange::of(paddr, sizeof(T)), flags, MemoryType::eUncached);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(MemoryRange range, PageFlags flags = detail::kDefaultPageFlags<T>) {
            return mapObject<T>(range, flags, MemoryType::eUncached);
        }
    };
}
