#pragma once

#include <bezos/subsystem/hid.h>

#include "fs2/device.hpp"
#include "fs2/identify.hpp"
#include "hid/hid.hpp"

#include "std/spinlock.hpp"
#include "std/shared_spinlock.hpp"

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

    class HidKeyboardHandle : public vfs2::IHandle {
        HidKeyboardDevice *mNode;
        stdx::SpinLock mLock;
        stdx::Vector2<OsHidEvent> mEvents;

    public:
        HidKeyboardHandle(HidKeyboardDevice *node);

        void notify(OsHidEvent event);

        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result) override;
        vfs2::HandleInfo info() override;
    };

    class HidKeyboardDevice
        : public vfs2::BasicNode
        , public vfs2::ConstIdentifyMixin<kHidInfo>
        , public km::ISubscriber
    {
        stdx::SharedSpinLock mLock;
        stdx::Vector2<HidKeyboardHandle*> mHandles;

        // km::ISubscriber interface
        void notify(km::Topic*, km::INotification *notification) override {
            hid::HidNotification *hid = static_cast<hid::HidNotification*>(notification);

            stdx::SharedLock guard(mLock);
            for (HidKeyboardHandle *handle : mHandles) {
                handle->notify(hid->event());
            }
        }

    public:
        HidKeyboardDevice() = default;

        void attach(HidKeyboardHandle *handle) {
            stdx::UniqueLock guard(mLock);
            mHandles.add(handle);
        }

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
    };
}
