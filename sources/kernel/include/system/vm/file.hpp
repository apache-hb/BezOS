#pragma once

#include "memory/layout.hpp"
#include "system/vmem.hpp"
#include "fs2/interface.hpp"

namespace sys2 {
    class FileMapping : public IMemoryObject {
        std::unique_ptr<vfs2::IFileHandle> mFileHandle;
        uint64_t mMappingFront;
        uint64_t mMappingBack;
        km::MemoryRange mRange;

    public:
        FileMapping(std::unique_ptr<vfs2::IFileHandle> fileHandle, uint64_t front, uint64_t back, km::MemoryRange range)
            : mFileHandle(std::move(fileHandle))
            , mMappingFront(front)
            , mMappingBack(back)
            , mRange(range)
        { }

        OsStatus fault(km::AnyRange<size_t> range, void *base, km::MemoryRange *result) override;

        OsStatus release(km::AnyRange<size_t> range) override;

        OsStatus flush(km::AnyRange<size_t> range) override;
    };

    OsStatus MapFileToMemory(std::unique_ptr<vfs2::IFileHandle> fileHandle, km::AddressMapping mapping, uint64_t front, uint64_t back, sm::RcuSharedPtr<FileMapping> outMapping);
}
