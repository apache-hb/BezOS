#pragma once

#include "fs/base.hpp"

#include "system/base.hpp"
#include "system/create.hpp"

namespace sys {
    class Device final : public BaseObject<eOsHandleDevice> {
        std::unique_ptr<vfs::IHandle> mVfsHandle;

    public:
        using Access = DeviceAccess;

        Device(std::unique_ptr<vfs::IHandle> handle);

        stdx::StringView getClassName() const override { return "Device"; }

        vfs::IHandle *getVfsHandle() { return mVfsHandle.get(); }
    };

    class DeviceHandle final : public BaseHandle<Device> {
    public:
        DeviceHandle(sm::RcuSharedPtr<Device> device, OsHandle handle, DeviceAccess access);

        sm::RcuSharedPtr<Device> getDevice() { return getInner(); }
    };
}
