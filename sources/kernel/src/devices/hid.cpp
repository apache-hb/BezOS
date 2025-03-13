#include "devices/hid.hpp"

#include "fs2/query.hpp"

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

static constexpr inline vfs2::InterfaceList kInterfaceList = std::to_array({
    vfs2::InterfaceOf<vfs2::TIdentifyHandle<dev::HidKeyboardDevice>, dev::HidKeyboardDevice>(kOsIdentifyGuid),
    vfs2::InterfaceOf<dev::HidKeyboardHandle, dev::HidKeyboardDevice>(kOsHidClassGuid),
});

OsStatus dev::HidKeyboardDevice::query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) {
    return kInterfaceList.query(this, uuid, data, size, handle);
}

OsStatus dev::HidKeyboardDevice::interfaces(OsIdentifyInterfaceList *list) {
    return kInterfaceList.list(list);
}
