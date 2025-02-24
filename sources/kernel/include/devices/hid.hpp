#pragma once

#include <bezos/subsystem/hid.h>

#include "fs2/device.hpp"
#include "hid/hid.hpp"

#include "std/spinlock.hpp"
#include "std/shared_spinlock.hpp"

#include "notify.hpp"

namespace dev {
    class HidKeyboardHandle : public vfs2::IVfsNodeHandle {
        stdx::SpinLock mLock;
        stdx::Vector2<OsHidEvent> mEvents;

    public:
        HidKeyboardHandle(vfs2::IVfsDevice *node);

        void notify(OsHidEvent event);

        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result) override;
    };

    class HidKeyboardDevice
        : public vfs2::IVfsDevice
        , public km::ISubscriber
    {
        stdx::SharedSpinLock mLock;
        stdx::Vector2<HidKeyboardHandle*> mHandles;

        // km::ISubscriber interface
        void notify(km::Topic*, sm::RcuSharedPtr<km::INotification> notification) override {
            hid::HidNotification *hid = static_cast<hid::HidNotification*>(notification.get());

            stdx::SharedLock guard(mLock);
            for (HidKeyboardHandle *handle : mHandles) {
                handle->notify(hid->event());
            }
        }

    public:
        HidKeyboardDevice() {
            addInterface<HidKeyboardHandle>(kOsHidClassGuid);

            mIdentifyInfo = OsIdentifyInfo {
                .DisplayName = "PS/2 Keyboard",
                .Model = "Generic 101/102-Key (Intl) PC",
                .DeviceVendor = "Generic",
                .FirmwareRevision = "Generic",
                .DriverVendor = "BezOS",
                .DriverVersion = OS_VERSION(1, 0, 0),
            };
        }

        void attach(HidKeyboardHandle *handle) {
            stdx::UniqueLock guard(mLock);
            mHandles.add(handle);
        }
    };
}
