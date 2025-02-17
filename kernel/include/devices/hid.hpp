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
        stdx::Vector2<hid::HidEvent> mEvents;

    public:
        HidKeyboardHandle(vfs2::IVfsNode *node)
            : vfs2::IVfsNodeHandle(node)
        { }

        void notify(hid::HidEvent event) {
            stdx::LockGuard guard(mLock);
            mEvents.add(event);
        }

        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result) override {
            if (request.size() <= 0 || request.offset != 0) {
                return OsStatusInvalidInput;
            }

            if (request.size() % sizeof(hid::HidEvent) != 0) {
                return OsStatusInvalidInput;
            }

            size_t count = std::min(request.size() / sizeof(hid::HidEvent), mEvents.count());
            stdx::LockGuard guard(mLock);
            std::memcpy(request.begin, mEvents.data(), count * sizeof(hid::HidEvent));
            mEvents.erase(0, count);

            result->read = count * sizeof(hid::HidEvent);
            return OsStatusSuccess;
        }
    };

    class HidKeyboardDevice
        : public vfs2::IVfsDevice
        , public km::ISubscriber
    {
        stdx::SharedSpinLock mLock;
        stdx::Vector2<HidKeyboardHandle*> mHandles;

        // vfs2::IVfsDevice interface
        OsStatus query(sm::uuid uuid, vfs2::IVfsNodeHandle **handle) override {
            if (uuid != kOsHidClassGuid) {
                return OsStatusNotSupported;
            }

            HidKeyboardHandle *hidHandle = new (std::nothrow) HidKeyboardHandle(this);
            if (!hidHandle) {
                return OsStatusOutOfMemory;
            }

            stdx::UniqueLock guard(mLock);

            mHandles.push_back(hidHandle);
            *handle = hidHandle;

            return OsStatusSuccess;
        }

        // km::ISubscriber interface
        void notify(km::Topic*, sm::RcuSharedPtr<km::INotification> notification) override {
            hid::HidNotification *hid = static_cast<hid::HidNotification*>(notification.get());
            hid::HidEvent event = hid->event();

            stdx::SharedLock guard(mLock);
            for (HidKeyboardHandle *handle : mHandles) {
                handle->notify(event);
            }
        }
    };
}
