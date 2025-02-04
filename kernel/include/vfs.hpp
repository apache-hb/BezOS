#pragma once

#include "vfs_types.hpp"

namespace km {
    class VirtualFileSystem {
        stdx::Vector2<VfsMount> mMounts;
        stdx::Vector2<VfsHandle> mHandles;
    public:
    };
}
