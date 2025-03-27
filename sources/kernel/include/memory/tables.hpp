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

    void copyHigherHalfMappings(PageTables *tables, const PageTables *source);

    class VmemAllocator {
        RangeAllocator<const std::byte*> mVmemAllocator;
    public:
        VmemAllocator() = default;
        VmemAllocator(VirtualRange vmemArea)
            : mVmemAllocator(vmemArea.cast<const std::byte*>())
        { }

        void reserve(VirtualRange range) {
            mVmemAllocator.reserve(range.cast<const std::byte*>());
        }

        VirtualRange allocate(size_t pages) {
            size_t align = (pages > (x64::kLargePageSize / x64::kPageSize)) ? x64::kLargePageSize : x64::kPageSize;
            return allocate({ .size = pages * x64::kPageSize, .align = align });
        }

        VirtualRange allocate(RangeAllocateRequest<const void*> request) {
            auto range = mVmemAllocator.allocate({ .size = request.size, .align = request.align, .hint = (const std::byte*)request.hint });
            return range.cast<const void*>();
        }

        void release(VirtualRange range) {
            mVmemAllocator.release(range.cast<const std::byte*>());
        }
    };

    class AddressSpaceAllocator {
        PageTables mTables;
        PageFlags mExtraFlags;
        RangeAllocator<const std::byte*> mVmemAllocator;

    protected:
        void init(AddressMapping pteMemory, const PageBuilder *pager, PageFlags flags, PageFlags extra, VirtualRange vmemArea);

    public:
        AddressSpaceAllocator() = default;

        AddressSpaceAllocator(AddressMapping pteMemory, const PageBuilder *pm, PageFlags flags, PageFlags extra, VirtualRange vmemArea);

        PageTables& ptes() { return mTables; }
        PhysicalAddress root() const { return mTables.root(); }
        const PageBuilder *pageManager() const { return mTables.pageManager(); }

        void reserve(VirtualRange range);
        VirtualRange vmemAllocate(size_t pages);
        VirtualRange vmemAllocate(RangeAllocateRequest<const void*> request);
        void vmemRelease(VirtualRange range);

        OsStatus map(AddressMapping mapping, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        /// @brief Map a physical memory range into the virtual memory space.
        ///
        /// @param range the physical memory range to map.
        /// @param flags the page flags to use.
        /// @param type the memory type to use.
        /// @param mapping the resulting virtual memory mapping.
        /// @return The status of the operation.
        OsStatus map(MemoryRange range, PageFlags flags, MemoryType type, AddressMapping *mapping);

        OsStatus unmap(VirtualRange range);
    };

    /// @brief Kernel address space.
    ///
    /// The top half of address space is reserved for the kernel.
    /// Each process has the kernel address space mapped into the higher half.
    /// Kernel addresses are identified by having the top bits set in the canonical address.
    class SystemPageTables : public AddressSpaceAllocator {
    public:
        SystemPageTables() = default;

        SystemPageTables(AddressMapping pteMemory, const PageBuilder *pm, VirtualRange systemArea);
    };

    /// @brief Per process address space.
    ///
    /// Each process has its own address space, which has the kernel address space mapped into the higher half.
    /// The lower half of the address space is reserved for the process.
    /// Process addresses are identified by having the top bits clear in the canonical address.
    class ProcessPageTables : public AddressSpaceAllocator {
        SystemPageTables *mSystemTables;

        void copyHigherHalfMappings();
    public:
        ProcessPageTables() = default;

        ProcessPageTables(SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea);

        void init(SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea);
    };
}
