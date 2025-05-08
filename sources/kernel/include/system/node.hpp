#pragma once

#include <bezos/facility/device.h>
#include <bezos/facility/node.h>

#include "system/base.hpp"
#include "system/create.hpp"

namespace vfs {
    class INode;
}

namespace sys {
    class Node final : public BaseObject<eOsHandleNode> {
        sm::RcuSharedPtr<vfs::INode> mVfsNode;
    public:
        using Access = NodeAccess;

        Node(sm::RcuSharedPtr<vfs::INode> vfsNode);

        stdx::StringView getClassName() const override { return "Node"; }

        sm::RcuSharedPtr<vfs::INode> getVfsNode() const { return mVfsNode; }
    };

    class NodeHandle final : public BaseHandle<Node> {
    public:
        NodeHandle(sm::RcuSharedPtr<Node> node, OsHandle handle, NodeAccess access);

        OsStatus clone(OsHandleAccess access, OsHandle id, IHandle **result) override;
        sm::RcuSharedPtr<Node> getNode() { return getInner(); }
    };
}
