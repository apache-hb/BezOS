#pragma once

#include <bezos/status.h>

#include "arch/paging.hpp"

#include "memory/layout.hpp"
#include "memory/pte.hpp"
#include "memory/range_allocator.hpp"
#include "memory/paging.hpp"

namespace km {
    struct AllocateRequest {
        size_t size;
        size_t align = x64::kPageSize;
        const void *hint = nullptr;
        PageFlags flags = PageFlags::eNone;
        MemoryType type = MemoryType::eWriteBack;
    };

    /// @brief Kernel address space.
    ///
    /// The top half of address space is reserved for the kernel.
    /// Each process has the kernel address space mapped into the higher half.
    /// Kernel addresses are identified by having the top bits set in the canonical address.
    class SystemPageTables {
        PageTables mSystemTables;
        RangeAllocator<const void*> mVmemAllocator;

    public:
        SystemPageTables(AddressMapping pteMemory, const km::PageBuilder *pm, VirtualRange systemArea)
            : mSystemTables(pm, pteMemory, PageFlags::eAll)
            , mVmemAllocator(systemArea)
        { }

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(AddressMapping mapping);

        PageTables& ptes() { return mSystemTables; }
        PhysicalAddress root() const { return mSystemTables.root(); }
    };

    /// @brief Per process address space.
    ///
    /// Each process has its own address space, which has the kernel address space mapped into the higher half.
    /// The lower half of the address space is reserved for the process.
    /// Process addresses are identified by having the top bits clear in the canonical address.
    class ProcessPageTables {
        SystemPageTables *mSystemTables;
        PageTables mProcessTables;
        RangeAllocator<const void*> mVmemAllocator;

    public:
        ProcessPageTables(SystemPageTables *kernel, AddressMapping pteMemory, const km::PageBuilder *pm, VirtualRange processArea)
            : mSystemTables(kernel)
            , mProcessTables(pm, pteMemory, PageFlags::eUserAll)
            , mVmemAllocator(processArea)
        {
            //
            // Copy the higher half mappings from the kernel ptes to the process ptes.
            //

            PageTables& system = mSystemTables->ptes();
            PageTables& process = mProcessTables;

            x64::PageMapLevel4 *pml4 = system.pml4();
            x64::PageMapLevel4 *self = process.pml4();

            //
            // Only copy the higher half mappings, which are 256-511.
            //
            static constexpr size_t kCount = (sizeof(x64::PageMapLevel4) / sizeof(x64::pml4e)) / 2;
            std::copy_n(pml4->entries + kCount, kCount, self->entries + kCount);
        }

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(AddressMapping mapping);

        OsStatus allocate(AllocateRequest request, AddressMapping *mapping);

        PhysicalAddress root() const { return mProcessTables.root(); }
    };
}
