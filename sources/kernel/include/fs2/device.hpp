#pragma once

#include <bezos/subsystem/identify.h>

#include "fs2/node.hpp"

namespace vfs2 {
    class VfsIdentifyHandle : public IHandle {
        IVfsNode *mNode;

        OsStatus serveInterfaceList(void *data, size_t size);
        OsStatus serveInfo(void *data, size_t size);

    public:
        VfsIdentifyHandle(IVfsNode *node)
            : mNode(node)
        { }

        OsStatus invoke(uint64_t function, void *data, size_t size) override;
        HandleInfo info() override { return HandleInfo { mNode }; }
    };

    class BasicNode : public INode {
    protected:
        INode *mParent;
        IVfsMount *mMount;
        VfsString mName;
        Access mAccess;

    public:
        BasicNode(INode *parent, IVfsMount *mount, VfsString name)
            : mParent(parent)
            , mMount(mount)
            , mName(std::move(name))
        { }

        BasicNode()
            : BasicNode(nullptr, nullptr, "")
        { }

        void init(INode *parent, VfsString name, Access access) override {
            mParent = parent;
            mName = std::move(name);
            mAccess = access;
        }

        NodeInfo info() override {
            return NodeInfo {
                .name = mName,
                .mount = mMount,
                .parent = mParent,
                .access = mAccess,
            };
        }
    };
}
