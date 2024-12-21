#pragma once

#include "memory/layout.hpp"

#include "memory/paging.hpp"

namespace km {
    enum class PageFlags {
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,

        eCode = eRead | eExecute,
        eData = eRead | eWrite,
        eAll = eRead | eWrite | eExecute,
    };

    constexpr PageFlags operator|(PageFlags lhs, PageFlags rhs) noexcept {
        return static_cast<PageFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    constexpr PageFlags operator&(PageFlags lhs, PageFlags rhs) noexcept {
        return static_cast<PageFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    class SystemAllocator {
        const SystemMemoryLayout *mLayout;

    public:
        SystemAllocator(const SystemMemoryLayout *layout) noexcept;

        VirtualAddress map4k(PhysicalAddress paddr, size_t pages, PageFlags flags) noexcept;
        void unmap4k(VirtualAddress vaddr, size_t pages) noexcept;

        VirtualAddress alloc4k(size_t pages, PageFlags flags) noexcept;
    };
}

/// @brief remap the kernel to replace the boot page tables.
/// @param pm the page manager
/// @param vmm the virtual memory manager
/// @param layout the system memory layout
/// @param address the address of the kernel
/// @param hhdm direct map offset
void KmMapKernel(
    const km::PageManager& pm,
    km::SystemAllocator& vmm,
    const km::SystemMemoryLayout& layout,
    limine_kernel_address_response address
) noexcept;
