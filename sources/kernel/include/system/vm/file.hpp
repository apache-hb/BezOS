#pragma once

#include "memory/layout.hpp"
#include "system/vmem.hpp"
#include "fs2/interface.hpp"

namespace km {
    class PageAllocator;
}

namespace sys2 {
    class FileMapping : public IMemoryObject {
        /// @brief Kernel side view of the memory
        km::AddressMapping mMapping;

    public:
        FileMapping(km::AddressMapping mapping)
            : mMapping(mapping)
        { }

        OsStatus fault(size_t page, km::PhysicalAddress *address) override {
            size_t offset = page * x64::kPageSize;
            if (offset >= mMapping.size) {
                return OsStatusOutOfBounds;
            }

            *address = mMapping.paddr + offset;
            return OsStatusSuccess;
        }

        OsStatus release(size_t) override {
            return OsStatusSuccess;
        }

        km::MemoryRange range() override {
            return mMapping.physicalRange();
        }
    };

    OsStatus MapFileToMemory(sm::RcuDomain *domain, vfs2::IFileHandle *fileHandle, km::PageAllocator *pmm, km::AddressSpace *ptes, uint64_t front, uint64_t back, sm::RcuSharedPtr<FileMapping> *outMapping);
}
