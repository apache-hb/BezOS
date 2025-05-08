#include "devices/stream.hpp"

#include "fs/identify.hpp"
#include "fs/stream.hpp"
#include "fs/query.hpp"

dev::StreamDevice::StreamDevice(size_t size)
    : mBuffer(new std::byte[size])
    , mQueue(mBuffer.get(), size)
{ }

OsStatus dev::StreamDevice::read(vfs::ReadRequest request, vfs::ReadResult *result) {
    stdx::LockGuard guard(mLock);

    size_t count = std::min<size_t>(request.size(), mQueue.count());
    for (size_t i = 0; i < count; i++) {
        ((std::byte*)request.begin)[i] = mQueue.pollFront();
    }

    result->read = count;
    return OsStatusSuccess;
}

OsStatus dev::StreamDevice::write(vfs::WriteRequest request, vfs::WriteResult *result) {
    stdx::LockGuard guard(mLock);

    size_t count = std::min<size_t>(request.size(), mQueue.capacity() - mQueue.count());
    for (size_t i = 0; i < count; i++) {
        mQueue.addBack(((std::byte*)request.begin)[i]);
    }

    result->write = count;
    return OsStatusSuccess;
}

static constexpr inline vfs::InterfaceList kInterfaceList = std::to_array({
    vfs::InterfaceOf<vfs::TIdentifyHandle<dev::StreamDevice>, dev::StreamDevice>(kOsIdentifyGuid),
    vfs::InterfaceOf<vfs::TStreamHandle<dev::StreamDevice>, dev::StreamDevice>(kOsStreamGuid),
});

OsStatus dev::StreamDevice::query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) {
    return kInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus dev::StreamDevice::interfaces(OsIdentifyInterfaceList *list) {
    return kInterfaceList.list(list);
}
