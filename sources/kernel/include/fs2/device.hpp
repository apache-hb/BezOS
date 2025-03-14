#pragma once

#include <atomic>

#include <bezos/subsystem/identify.h>

#include "fs2/node.hpp"

namespace vfs2 {
    class BasicNode : public INode {
        std::atomic<unsigned> mPublicRefCount { 1 };

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

        void retain() override {
            mPublicRefCount.fetch_add(1, std::memory_order_relaxed);
        }

        unsigned release() override {
            unsigned count = mPublicRefCount.fetch_sub(1, std::memory_order_acq_rel);
            if (count == 1) {
                delete this;
            }

            return count - 1;
        }
    };

    template<std::derived_from<INode> Node, std::derived_from<IHandle> Interface = IHandle>
    class BasicHandle : public Interface {
    protected:
        Node *mNode;

    public:
        BasicHandle(Node *node)
            : mNode(node)
        {
            mNode->retain();
        }

        virtual ~BasicHandle() override {
            mNode->release();
        }
    };
}
