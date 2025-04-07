#pragma once

#include <bezos/facility/device.h>
#include <bezos/facility/fs.h>

#include "system/handle.hpp"
#include "system/create.hpp"

namespace sys2 {
    class DeviceHandle final : public IHandle {
        sm::RcuSharedPtr<Device> mDevice;
        OsHandle mHandle;
        DeviceAccess mAccess;

    public:
        sm::RcuSharedPtr<Device> getDevice() { return mDevice; }

        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }
    };

    class Device final : public IObject {
        stdx::SharedSpinLock mLock;

        ObjectName mName GUARDED_BY(mLock);
        sm::RcuWeakPtr<Node> mNode GUARDED_BY(mLock);

        sm::RcuWeakPtr<DeviceHandle> mHandle GUARDED_BY(mLock);

    public:
        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Device"; }
    };

    class NodeHandle final : public IHandle {
        sm::RcuWeakPtr<Node> mNode;
        OsHandle mHandle;
        NodeAccess mAccess;
    public:
        sm::RcuWeakPtr<Node> getNode() { return mNode; }

        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }
    };

    class Node final : public IObject {
        stdx::SharedSpinLock mLock;

        ObjectName mName GUARDED_BY(mLock);
        sm::RcuWeakPtr<Node> mParent GUARDED_BY(mLock);
        sm::RcuWeakPtr<NodeHandle> mHandle GUARDED_BY(mLock);
    public:
        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Node"; }
    };
}
