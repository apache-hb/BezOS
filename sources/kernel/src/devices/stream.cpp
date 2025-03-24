#include "devices/stream.hpp"

#include "fs2/identify.hpp"
#include "fs2/stream.hpp"
#include "fs2/query.hpp"
#include "log.hpp"

dev::StreamDevice::StreamDevice(size_t size)
    : mBuffer(new std::byte[size])
    , mQueue(mBuffer.get(), size)
{ }

OsStatus dev::StreamDevice::read(vfs2::ReadRequest request, vfs2::ReadResult *result) {
    stdx::LockGuard guard(mLock);

    size_t count = std::min<size_t>(request.size(), mQueue.count());
    for (size_t i = 0; i < count; i++) {
        ((std::byte*)request.begin)[i] = mQueue.pollFront();
        KmDebugMessage("[STREAM] Reading: ", (int)((std::byte*)request.begin)[i], " (", (char)((std::byte*)request.begin)[i], ")\n");
    }

    result->read = count;
    return OsStatusSuccess;
}

OsStatus dev::StreamDevice::write(vfs2::WriteRequest request, vfs2::WriteResult *result) {
    stdx::LockGuard guard(mLock);

    size_t count = std::min<size_t>(request.size(), mQueue.capacity() - mQueue.count());
    for (size_t i = 0; i < count; i++) {
        KmDebugMessage("[STREAM] Writing: ", (int)((std::byte*)request.begin)[i], " (", (char)((std::byte*)request.begin)[i], ")\n");
        mQueue.addBack(((std::byte*)request.begin)[i]);
    }

    result->write = count;
    return OsStatusSuccess;
}

static constexpr inline vfs2::InterfaceList kInterfaceList = std::to_array({
    vfs2::InterfaceOf<vfs2::TIdentifyHandle<dev::StreamDevice>, dev::StreamDevice>(kOsIdentifyGuid),
    vfs2::InterfaceOf<vfs2::TStreamHandle<dev::StreamDevice>, dev::StreamDevice>(kOsStreamGuid),
});

OsStatus dev::StreamDevice::query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) {
    return kInterfaceList.query(this, uuid, data, size, handle);
}

OsStatus dev::StreamDevice::interfaces(OsIdentifyInterfaceList *list) {
    return kInterfaceList.list(list);
}
