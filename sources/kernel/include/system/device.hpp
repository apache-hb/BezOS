#pragma once

#include <bezos/facility/device.h>
#include <bezos/facility/fs.h>

#include "fs2/base.hpp"

#include "system/base.hpp"
#include "system/create.hpp"

namespace sys2 {
    class Device final : public BaseObject<eOsHandleDevice> {
        std::unique_ptr<vfs2::IHandle> mVfsHandle;

    public:
        using Access = DeviceAccess;

        Device(std::unique_ptr<vfs2::IHandle> handle);

        stdx::StringView getClassName() const override { return "Device"; }

        vfs2::IHandle *getVfsHandle() { return mVfsHandle.get(); }
    };

    class DeviceHandle final : public BaseHandle<Device, eOsHandleDevice> {
    public:
        DeviceHandle(sm::RcuSharedPtr<Device> device, OsHandle handle, DeviceAccess access);

        sm::RcuSharedPtr<Device> getDevice() { return getInner(); }
    };
}
