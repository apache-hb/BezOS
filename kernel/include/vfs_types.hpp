#pragma once

#include "std/string.hpp"
#include "std/string_view.hpp"

#include <utility>

#include "absl/container/btree_map.h"

namespace km {
    class IFileSystem;
    enum class VfsNodeId : std::uint64_t {};

    enum class VfsNodeType {
        eFile,
        eFolder,
        eDevice,
        eMount,
    };

    class VfsHandle {
        IFileSystem *mDriver;
        VfsNodeId mNodeId;

    public:
        VfsHandle(IFileSystem *driver, VfsNodeId nodeId)
            : mDriver(driver)
            , mNodeId(nodeId)
        { }

        IFileSystem *driver() const { return mDriver; }
        VfsNodeId nodeId() const { return mNodeId; }
        VfsNodeType type() const;
    };

    class VfsMount {
        stdx::String mName;
        IFileSystem *mDriver;

    public:
        VfsMount(stdx::String name, IFileSystem *driver)
            : mName(std::move(name))
            , mDriver(driver)
        { }

        stdx::StringView name() const { return mName; }
        IFileSystem *driver() const { return mDriver; }
    };
}
