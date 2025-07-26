#pragma once

#include "memory/address_space.hpp"
#include "memory/layout.hpp"
#include "system/vmem.hpp"
#include "fs/interface.hpp"

namespace km {
    class PageAllocator;
}

namespace sys {
    class FileMapping : public IMemoryObject {
        /// @brief Kernel side view of the memory
        km::AddressMapping mMapping;

    public:
        FileMapping(km::AddressMapping mapping)
            : mMapping(mapping)
        { }

        OsStatus release(size_t) override {
            return OsStatusSuccess;
        }

        km::MemoryRange range() override {
            return mMapping.physicalRange();
        }
    };

    OsStatus MapFileToMemory(sm::RcuDomain *domain, vfs::IFileHandle *fileHandle, km::PageAllocator *pmm, km::AddressSpace *ptes, uint64_t front, uint64_t back, sm::RcuSharedPtr<FileMapping> *outMapping);
}
