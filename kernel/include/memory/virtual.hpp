#pragma once

#include "memory/physical.hpp"

#include "arch/paging.hpp"

namespace km {
    class PhysicalAllocator {
        SystemMemoryLayout *mLayout;

        uint8_t *mBitMap;
        size_t mCount; // number of bytes in the bitmap

        void markPagesUsed(size_t first, size_t last) noexcept;
        void markUsed(MemoryRange range) noexcept;

        PhysicalAddress getAddress(size_t index) const noexcept;

    public:
        PhysicalAllocator(SystemMemoryLayout *layout) noexcept;

        PhysicalAddress alloc4k(size_t pages) noexcept;
        void release4k(PhysicalAddress address, size_t pages) noexcept;
    };

    enum class PageFlags {
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,

        eCode = eRead | eExecute,
        eData = eRead | eWrite,
    };

    class PageAllocator {
        PhysicalAllocator *mMemory;
        x64::PageMapLevel4 *mPageMap;

    public:
        PageAllocator(PhysicalAllocator *pmm) noexcept;

        PhysicalAddress toPhysical(VirtualAddress address) const noexcept;
        VirtualAddress toVirtual(PhysicalAddress address) const noexcept;

        void map4k(VirtualAddress vaddr, PhysicalAddress paddr, size_t pages, PageFlags flags) noexcept;
        void unmap4k(VirtualAddress vaddr, size_t pages) noexcept;
    };
}

void KmRemapKernel(km::PageAllocator& vmm, limine_kernel_address_response address) noexcept;
