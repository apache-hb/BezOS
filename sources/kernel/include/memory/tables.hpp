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

    class IPageTables {
        PageTables mTables;
        RangeAllocator<const std::byte*> mVmemAllocator;

    public:
        IPageTables(AddressMapping pteMemory, const PageBuilder *pm, PageFlags flags, VirtualRange vmemArea)
            : mTables(pm, pteMemory, flags)
            , mVmemAllocator(vmemArea.cast<const std::byte*>())
        { }

        PageTables& ptes() { return mTables; }
        PhysicalAddress root() const { return mTables.root(); }
        const PageBuilder *pageManager() const { return mTables.pageManager(); }

        void reserve(VirtualRange range) {
            mVmemAllocator.markUsed(range.cast<const std::byte*>());
        }

        VirtualRange vmemAllocate(size_t pages) {
            size_t align = (pages > (x64::kLargePageSize / x64::kPageSize)) ? x64::kLargePageSize : x64::kPageSize;
            auto range = mVmemAllocator.allocate({
                .size = pages * x64::kPageSize,
                .align = align,
            });

            return range.cast<const void*>();
        }

        OsStatus map(AddressMapping mapping, PageFlags flags, MemoryType type = MemoryType::eWriteBack) {
            return mTables.map(mapping, flags, type);
        }

        OsStatus map(MemoryRange range, PageFlags flags, MemoryType type, AddressMapping *mapping) {
            size_t pages = Pages(range.size());
            VirtualRange vmem = vmemAllocate(pages);
            if (vmem.isEmpty()) {
                return OsStatusOutOfMemory;
            }

            AddressMapping m = MappingOf(vmem, range.front);
            OsStatus status = map(m, flags, type);
            if (status != OsStatusSuccess) {
                return status;
            }

            *mapping = m;
            return OsStatusSuccess;
        }

        OsStatus unmap(VirtualRange range) {
            mTables.unmap(range);
            mVmemAllocator.release(range.cast<const std::byte*>());
            return OsStatusSuccess;
        }
    };

    /// @brief Kernel address space.
    ///
    /// The top half of address space is reserved for the kernel.
    /// Each process has the kernel address space mapped into the higher half.
    /// Kernel addresses are identified by having the top bits set in the canonical address.
    class SystemPageTables : public IPageTables {

    public:
        SystemPageTables(AddressMapping pteMemory, const PageBuilder *pm, VirtualRange systemArea)
            : IPageTables(pteMemory, pm, PageFlags::eAll, systemArea)
        { }
    };

    /// @brief Per process address space.
    ///
    /// Each process has its own address space, which has the kernel address space mapped into the higher half.
    /// The lower half of the address space is reserved for the process.
    /// Process addresses are identified by having the top bits clear in the canonical address.
    class ProcessPageTables : public IPageTables {
        SystemPageTables *mSystemTables;

    public:
        ProcessPageTables(SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea)
            : IPageTables(pteMemory, kernel->pageManager(), PageFlags::eUserAll, processArea)
            , mSystemTables(kernel)
        {
            //
            // Copy the higher half mappings from the kernel ptes to the process ptes.
            //

            PageTables& system = mSystemTables->ptes();
            PageTables& process = ptes();

            x64::PageMapLevel4 *pml4 = system.pml4();
            x64::PageMapLevel4 *self = process.pml4();

            //
            // Only copy the higher half mappings, which are 256-511.
            //
            static constexpr size_t kCount = (sizeof(x64::PageMapLevel4) / sizeof(x64::pml4e)) / 2;
            std::copy_n(pml4->entries + kCount, kCount, self->entries + kCount);
        }
    };
}
