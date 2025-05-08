#pragma once

#include "process/object.hpp"

#include "fs/base.hpp"
#include "std/string.hpp"

namespace km {
    struct Node : public BaseObject {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;

        void init(NodeId id, stdx::String name, sm::RcuSharedPtr<vfs::INode> vfsNode);
    };

    struct Device : public BaseObject {
        std::unique_ptr<vfs::IHandle> handle;

        void init(DeviceId id, stdx::String name, std::unique_ptr<vfs::IHandle> vfsHandle);
    };
}
