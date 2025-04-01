#pragma once

#include <bezos/facility/device.h>
#include <bezos/facility/fs.h>

#include "system/handle.hpp"

namespace sys2 {
    class System;
    class Node;
    class NodeHandle;
    class Device;
    class DeviceHandle;

    enum class DeviceAccess : OsHandleAccess {
        eNone = eOsDeviceAccessNone,
        eRead = eOsDeviceAccessRead,
        eWrite = eOsDeviceAccessWrite,
        eStat = eOsDeviceAccessStat,
        eDestroy = eOsDeviceAccessDestroy,
        eInvoke = eOsDeviceAccessInvoke,
        eAll = eOsDeviceAccessAll,
    };

    class DeviceHandle final : public IHandle {
        sm::RcuWeakPtr<Device> mDevice;
        OsHandle mHandle;
        DeviceAccess mAccess;

    public:
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

    enum class NodeAccess : OsHandleAccess {
        eNone = eOsNodeAccessNone,
        eRead = eOsNodeAccessRead,
        eWrite = eOsNodeAccessWrite,
        eStat = eOsNodeAccessStat,
        eDestroy = eOsNodeAccessDestroy,
        eQueryInterface = eOsNodeAccessQueryInterface,
        eAll = eOsNodeAccessAll,
    };

    class NodeHandle final : public IHandle {
        sm::RcuWeakPtr<Node> mNode;
        OsHandle mHandle;
        NodeAccess mAccess;
    public:
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
