#pragma once

#include <bezos/status.h>

#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"
#include "arch/paging.hpp"

#include "memory/page_allocator.hpp"
#include "memory/range_allocator.hpp"
#include "memory/paging.hpp"

#include "std/spinlock.hpp"

namespace km {
    struct MappingRequest {
        AddressMapping mapping;
        PageFlags flags = PageFlags::eNone;
        MemoryType type = MemoryType::eWriteBack;
    };

    class PageTables {
        uintptr_t mSlide = 0;
        mem::SynchronizedAllocator<mem::TlsfAllocator> mAllocator;
        const km::PageBuilder *mPageManager;
        x64::PageMapLevel4 *mRootPageTable;
        stdx::SpinLock mLock;

        x64::page *allocPage() [[clang::allocating]];

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address, bool present) noexcept [[clang::nonblocking]];

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, PageFlags flags);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, PageFlags flags);
        x64::PageTable *getPageTable(x64::PageMapLevel2 *l2, uint16_t pdte, PageFlags flags);

        const x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const noexcept [[clang::nonblocking]];
        const x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const noexcept [[clang::nonblocking]];
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const noexcept [[clang::nonblocking]];

        template<typename T>
        T *asVirtual(km::PhysicalAddress addr) const noexcept [[clang::nonblocking]] {
            return (T*)(addr.address + mSlide);
        }

        PhysicalAddress asPhysical(const void *ptr) const noexcept [[clang::nonblocking]];

        OsStatus map4k(const void *vaddr, PhysicalAddress paddr, PageFlags flags, MemoryType type, PageFlags middle);
        OsStatus map2m(const void *vaddr, PhysicalAddress paddr, PageFlags flags, MemoryType type, PageFlags middle);

    public:
        PageTables(AddressMapping pteMemory, const km::PageBuilder *pm);

        const x64::PageMapLevel4 *root() const {
            return mRootPageTable;
        }

        km::PhysicalAddress rootPageTable() const {
            return km::PhysicalAddress { (uintptr_t)mRootPageTable - mSlide };
        }

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(VirtualRange range) noexcept;
    };

    /// @brief Kernel address space.
    ///
    /// The top half of address space is reserved for the kernel.
    /// Each process has the kernel address space mapped into the higher half.
    /// Kernel addresses are identified by having the top bits set in the canonical address.
    class KernelPageTables {
        PageTables mSystemTables;
        RangeAllocator<const std::byte*> mVmemAllocator;

    public:
        KernelPageTables();

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(AddressMapping mapping);
    };

    /// @brief Per process address space.
    ///
    /// Each process has its own address space, which has the kernel address space mapped into the higher half.
    /// The lower half of the address space is reserved for the process.
    /// Process addresses are identified by having the top bits clear in the canonical address.
    class ProcessPageTables {
        KernelPageTables *mSystemTables;
        PageTables mProcessTables;
        RangeAllocator<const std::byte*> mVmemAllocator;

    public:
        ProcessPageTables(KernelPageTables *kernel);

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(AddressMapping mapping);
    };
}
