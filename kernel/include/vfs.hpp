#pragma once

#include "drivers/fs/driver.hpp"
#include "std/string.hpp"
#include "std/string_view.hpp"
#include "std/vector.hpp"

#include <new>
#include <utility>

namespace km {
    enum class VfsNodeId : std::uint64_t {};

    class VfsHandle {
        IFileSystem *mDriver;
        VfsNodeId mNodeId;
    };

    class VfsMount {
        stdx::String mName;
        IFileSystem *mDriver;
    };

    class VirtualFileSystem {
        stdx::Vector2<VfsMount> mMounts;
        stdx::Vector2<VfsHandle> mHandles;
    public:
    };
}
