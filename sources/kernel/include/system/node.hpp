#pragma once

#include <bezos/facility/device.h>
#include <bezos/facility/fs.h>

#include "system/base.hpp"
#include "system/create.hpp"

namespace vfs2 {
    class INode;
}

namespace sys2 {
    class Node final : public BaseObject<eOsHandleNode> {
        sm::RcuSharedPtr<vfs2::INode> mVfsNode;
    public:
        using Access = NodeAccess;

        Node(sm::RcuSharedPtr<vfs2::INode> vfsNode);

        stdx::StringView getClassName() const override { return "Node"; }

        sm::RcuSharedPtr<vfs2::INode> getVfsNode() const { return mVfsNode; }
    };

    class NodeHandle final : public BaseHandle<Node, eOsHandleNode> {
    public:
        NodeHandle(sm::RcuSharedPtr<Node> node, OsHandle handle, NodeAccess access);

        OsStatus clone(OsHandleAccess access, OsHandle id, IHandle **result) override;
        sm::RcuSharedPtr<Node> getNode() { return getInner(); }
    };
}
