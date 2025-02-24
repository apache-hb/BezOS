#include "devices/hid.hpp"

dev::HidKeyboardHandle::HidKeyboardHandle(vfs2::IVfsDevice *node)
    : vfs2::IVfsNodeHandle(node)
{
    static_cast<HidKeyboardDevice*>(node)->attach(this);
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
