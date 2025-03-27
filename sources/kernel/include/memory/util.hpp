#pragma once

#include "memory/memory.hpp"
#include "memory/range.hpp"

namespace km {
    class PageTables;

    void VirtualUnmap(PageTables& vmm, MemoryRange range);

    [[nodiscard]]
    void *VirtualMap(PageTables& vmm, MemoryRange range, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

    template<typename T>
    [[nodiscard]]
    T *VirtualMapMmio(PageTables& vmm, MemoryRange range, PageFlags flags = PageFlags::eData) {
        return (T*)VirtualMap(vmm, range, flags, MemoryType::eUncached);
    }

    template<typename T>
    [[nodiscard]]
    T *VirtualMapMmio(PageTables& vmm, PhysicalAddress paddr, PageFlags flags = PageFlags::eData) {
        return VirtualMapMmio<T>(vmm, MemoryRange::of(paddr, sizeof(T)), flags);
    }

    template<typename T>
    [[nodiscard]]
    T *VirtualMapObject(PageTables& vmm, MemoryRange range, PageFlags flags, MemoryType type) {
        return (T*)VirtualMap(vmm, range, flags, type);
    }

    template<typename T>
    [[nodiscard]]
    T *VirtualMapObject(PageTables& vmm, PhysicalAddress paddr, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack) {
        return VirtualMapObject<T>(vmm, MemoryRange::of(paddr, sizeof(T)), flags, type);
    }

    template<typename T>
    [[nodiscard]]
    const T *VirtualMapConst(PageTables& vmm, MemoryRange range, MemoryType type = MemoryType::eWriteBack) {
        return VirtualMapObject<const T>(vmm, range, PageFlags::eRead, type);
    }

    template<typename T>
    [[nodiscard]]
    const T *VirtualMapConst(PageTables& vmm, PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack) {
        return VirtualMapConst<T>(vmm, MemoryRange::of(paddr, sizeof(T)), type);
    }
}
