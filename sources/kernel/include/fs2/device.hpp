#pragma once

#include <atomic>

#include <bezos/subsystem/identify.h>

#include "fs2/node.hpp"

namespace vfs2 {
    class BasicNode : public INode {
        std::atomic<unsigned> mPublicRefCount { 1 };

    protected:
        sm::RcuWeakPtr<INode> mParent;
        IVfsMount *mMount;
        VfsString mName;
        sys2::NodeAccess mAccess;

    public:
        BasicNode(sm::RcuWeakPtr<INode> parent, IVfsMount *mount, VfsString name)
            : mParent(parent)
            , mMount(mount)
            , mName(std::move(name))
        { }

        BasicNode()
            : BasicNode(nullptr, nullptr, "")
        { }

        void init(sm::RcuWeakPtr<INode> parent, VfsString name, sys2::NodeAccess access) override {
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

    template<std::derived_from<INode> Node, std::derived_from<IHandle> Interface = IHandle>
    class BasicHandle : public Interface {
    protected:
        sm::RcuSharedPtr<Node> mNode;

    public:
        BasicHandle(sm::RcuSharedPtr<Node> node)
            : mNode(node)
        { }

        virtual ~BasicHandle() = default;
    };
}
