#pragma once

#include "fs2/node.hpp"

#include "std/fixed_deque.hpp"
#include "std/spinlock.hpp"

namespace dev {
    class StreamDevice;
    class StreamHandle;

    class StreamDevice : public vfs2::IVfsNode {
        std::unique_ptr<std::byte[]> mBuffer;
        stdx::SpinLock mLock;
        stdx::FixedSizeDeque<std::byte> mQueue;

    public:
        StreamDevice(size_t size);

        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result) override;
        OsStatus write(vfs2::WriteRequest request, vfs2::WriteResult *result) override;
    };
}
