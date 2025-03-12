#include "devices/hid.hpp"

dev::HidKeyboardHandle::HidKeyboardHandle(HidKeyboardDevice *node)
    : mNode(node)
{
    mNode->attach(this);
}

vfs2::HandleInfo dev::HidKeyboardHandle::info() {
    return vfs2::HandleInfo { mNode };
}

void dev::HidKeyboardHandle::notify(OsHidEvent event) {
    stdx::LockGuard guard(mLock);
    mEvents.add(event);
}

OsStatus dev::HidKeyboardHandle::read(vfs2::ReadRequest request, vfs2::ReadResult *result) {
    if (request.size() <= 0 || request.offset != 0) {
        return OsStatusInvalidInput;
    }

    if (request.size() % sizeof(OsHidEvent) != 0) {
        return OsStatusInvalidInput;
    }

    size_t count = std::min(request.size() / sizeof(OsHidEvent), mEvents.count());
    stdx::LockGuard guard(mLock);

    if (!mEvents.isEmpty()) {
        std::memcpy(request.begin, mEvents.data(), count * sizeof(OsHidEvent));
        mEvents.erase(0, count);
    }

    result->read = count * sizeof(OsHidEvent);
    return OsStatusSuccess;
}

OsStatus dev::HidKeyboardDevice::query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) {
    if (uuid == kOsIdentifyGuid) {
        auto *identify = new(std::nothrow) vfs2::TIdentifyHandle<HidKeyboardDevice>(this);
        if (!identify) {
            return OsStatusOutOfMemory;
        }

        *handle = identify;
        return OsStatusSuccess;
    }

    if (uuid == kOsHidClassGuid) {
        auto *hid = new(std::nothrow) HidKeyboardHandle(this);
        if (!hid) {
            return OsStatusOutOfMemory;
        }

        *handle = hid;
        return OsStatusSuccess;
    }

    return OsStatusInterfaceNotSupported;
}
