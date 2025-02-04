#pragma once

#include "drivers/fs/driver.hpp"

namespace km {
    class TmpFileSystem : public IFileSystem {
    public:
        TmpFileSystem() = default;

        KmStatus open(VfsHandle parent, stdx::StringView name, VfsHandle *handle);
        KmStatus close(VfsHandle handle);
        KmStatus unlink(VfsHandle parent, stdx::StringView name);

        KmStatus getNodeType(VfsHandle handle, VfsNodeType *type);
    };
}
