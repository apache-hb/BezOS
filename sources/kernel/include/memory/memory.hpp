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

    enum class PageSize2 {
        eNone,
        eRegular,
        eLarge,
        eHuge,
    };

    class PageTables {
        uintptr_t mSlide;
        mem::SynchronizedAllocator<mem::TlsfAllocator> mAllocator;
        const km::PageBuilder *mPageManager;
        x64::PageMapLevel4 *mRootPageTable;
        stdx::SpinLock mLock;

        x64::page *allocPage();

        void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address);

        x64::PageMapLevel3 *getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, PageFlags flags);
        x64::PageMapLevel2 *getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, PageFlags flags);

        const x64::PageMapLevel3 *findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const;
        const x64::PageMapLevel2 *findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const;
        x64::PageTable *findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const;

        OsStatus mapRange4k(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);
        OsStatus mapRange2m(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);
        OsStatus mapRange1g(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type);

        x64::PageMapLevel4 *getRootTable() const {
            return mRootPageTable;
        }

        template<typename T>
        T *asVirtual(km::PhysicalAddress addr) const {
            return (T*)(addr.address + mSlide);
        }

        PhysicalAddress asPhysical(const void *ptr) const;

        OsStatus map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        OsStatus map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        OsStatus map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        OsStatus mapRange(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type = MemoryType::eWriteBack);

        void unmap(void *ptr, size_t size);

    public:
        PageTables(AddressMapping pteMemory, const km::PageBuilder *pm);

        PhysicalAddress root() const {
            return asPhysical(getRootTable());
        }

        const x64::PageMapLevel4 *pml4() const {
            return mRootPageTable;
        }

        PhysicalAddress getBackingAddress(const void *ptr) const;
        PageFlags getMemoryFlags(const void *ptr) const;
        PageSize2 getPageSize(const void *ptr) const;

        OsStatus map(MappingRequest request, AddressMapping *mapping);
        OsStatus unmap(AddressMapping mapping);
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
