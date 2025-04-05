#pragma once

#include "process/object.hpp"

#include "fs2/base.hpp"
#include "std/string.hpp"

namespace km {
    struct Node : public KernelObject {
        sm::RcuSharedPtr<vfs2::INode> node = nullptr;

        void init(NodeId id, stdx::String name, sm::RcuSharedPtr<vfs2::INode> vfsNode);
    };

    struct Device : public KernelObject {
        std::unique_ptr<vfs2::IHandle> handle;

        void init(DeviceId id, stdx::String name, std::unique_ptr<vfs2::IHandle> vfsHandle);
    };
}
