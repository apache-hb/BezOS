#include "devices/stream.hpp"
#include "fs2/identify.hpp"
#include "fs2/stream.hpp"

dev::StreamDevice::StreamDevice(size_t size)
    : mBuffer(new std::byte[size])
    , mQueue(mBuffer.get(), size)
{ }

OsStatus dev::StreamDevice::read(vfs2::ReadRequest request, vfs2::ReadResult *result) {
    stdx::LockGuard guard(mLock);

    size_t count = std::min<size_t>(request.size(), mQueue.count());
    for (size_t i = 0; i < count; ++i) {
        ((std::byte*)request.begin)[i] = mQueue.pollFront();
    }

    result->read = count;
    return OsStatusSuccess;
}

OsStatus dev::StreamDevice::write(vfs2::WriteRequest request, vfs2::WriteResult *result) {
    stdx::LockGuard guard(mLock);

    size_t count = std::min<size_t>(request.size(), mQueue.capacity() - mQueue.count());
    for (size_t i = 0; i < count; ++i) {
        mQueue.addBack(((std::byte*)request.begin)[i]);
    }

    result->write = count;
    return OsStatusSuccess;
}

OsStatus dev::StreamDevice::query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) {
    if (uuid == kOsIdentifyGuid) {
        auto *identify = new(std::nothrow) vfs2::TIdentifyHandle<StreamDevice>(this);
        if (!identify) {
            return OsStatusOutOfMemory;
        }

        *handle = identify;
        return OsStatusSuccess;
    }

    if (uuid == kOsStreamGuid) {
        auto *stream = new(std::nothrow) vfs2::TStreamHandle<StreamDevice>(this);
        if (!stream) {
            return OsStatusOutOfMemory;
        }

        *handle = stream;
        return OsStatusSuccess;
    }

    return OsStatusInterfaceNotSupported;
}
