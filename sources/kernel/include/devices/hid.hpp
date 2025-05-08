#pragma once

#include <bezos/subsystem/hid.h>

#include "fs/device.hpp"
#include "fs/identify.hpp"

#include "std/spinlock.hpp"
#include "std/shared_spinlock.hpp"
#include "std/vector.hpp"

#include "notify.hpp"

namespace dev {
    class HidKeyboardDevice;
    class HidKeyboardHandle;

    static constexpr inline OsIdentifyInfo kHidInfo {
        .DisplayName = "PS/2 Keyboard",
        .Model = "Generic 101/102-Key (Intl) PC",
        .DeviceVendor = "Generic",
        .FirmwareRevision = "Generic",
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    class HidKeyboardDevice
        : public vfs::BasicNode
        , public vfs::ConstIdentifyMixin<kHidInfo>
        , public km::ISubscriber
    {
        stdx::SharedSpinLock mLock;
        stdx::Vector2<HidKeyboardHandle*> mHandles;

        // km::ISubscriber interface
        void notify(km::Topic*, km::INotification *notification) override;

    public:
        HidKeyboardDevice() = default;

        void attach(HidKeyboardHandle *handle);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
    };

    class HidKeyboardHandle : public vfs::BaseHandle<HidKeyboardDevice> {
        using vfs::BaseHandle<HidKeyboardDevice>::mNode;
        stdx::SpinLock mLock;
        stdx::Vector2<OsHidEvent> mEvents;

    public:
        HidKeyboardHandle(sm::RcuSharedPtr<HidKeyboardDevice> node, const void *data, size_t size);

        void notify(OsHidEvent event);

        OsStatus read(vfs::ReadRequest request, vfs::ReadResult *result) override;
        vfs::HandleInfo info() override;
    };
}
