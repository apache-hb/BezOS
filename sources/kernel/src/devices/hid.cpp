#include "devices/hid.hpp"

#include "fs/query.hpp"
#include "hid/hid.hpp"

//
// Handle
//

dev::HidKeyboardHandle::HidKeyboardHandle(sm::RcuSharedPtr<HidKeyboardDevice> node, const void *, size_t)
    : vfs::BaseHandle<HidKeyboardDevice>(node)
{
    mNode->attach(this);
}

vfs::HandleInfo dev::HidKeyboardHandle::info() {
    return vfs::HandleInfo { mNode, kOsHidClassGuid };
}

void dev::HidKeyboardHandle::notify(OsHidEvent event) {
    stdx::LockGuard guard(mLock);
    mEvents.add(event);
}

OsStatus dev::HidKeyboardHandle::read(vfs::ReadRequest request, vfs::ReadResult *result) {
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

//
// Device node
//

void dev::HidKeyboardDevice::notify(km::Topic*, km::INotification *notification) {
    hid::HidNotification *hid = static_cast<hid::HidNotification*>(notification);

    stdx::SharedLock guard(mLock);
    for (HidKeyboardHandle *handle : mHandles) {
        handle->notify(hid->event());
    }
}

void dev::HidKeyboardDevice::attach(HidKeyboardHandle *handle) {
    stdx::UniqueLock guard(mLock);
    mHandles.add(handle);
}

static constexpr inline vfs::InterfaceList kInterfaceList = std::to_array({
    vfs::InterfaceOf<vfs::TIdentifyHandle<dev::HidKeyboardDevice>, dev::HidKeyboardDevice>(kOsIdentifyGuid),
    vfs::InterfaceOf<dev::HidKeyboardHandle, dev::HidKeyboardDevice>(kOsHidClassGuid),
});

OsStatus dev::HidKeyboardDevice::query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) {
    return kInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus dev::HidKeyboardDevice::interfaces(OsIdentifyInterfaceList *list) {
    return kInterfaceList.list(list);
}
