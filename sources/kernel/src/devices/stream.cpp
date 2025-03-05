#include "devices/stream.hpp"

dev::StreamDevice::StreamDevice(size_t size)
    : mBuffer(new std::byte[size])
    , mQueue(mBuffer.get(), size)
{
    addInterface<vfs2::IVfsNodeHandle>(kOsStreamGuid);
}

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
