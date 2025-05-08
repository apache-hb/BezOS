#pragma once

#include "fs/device.hpp"
#include "fs/identify.hpp"

#include "std/fixed_deque.hpp"
#include "std/spinlock.hpp"

namespace dev {
    class StreamDevice;
    class StreamHandle;

    static constexpr inline OsIdentifyInfo kStreamInfo {
        .DisplayName = "Named pipe",
        .Model = "Generic",
        .DeviceVendor = "BezOS",
        .FirmwareRevision = "1.0.0",
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    class StreamDevice : public vfs::BasicNode, public vfs::ConstIdentifyMixin<kStreamInfo> {
        std::unique_ptr<std::byte[]> mBuffer;
        stdx::SpinLock mLock;
        stdx::FixedSizeDeque<std::byte> mQueue;

    public:
        StreamDevice(size_t size);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);

        OsStatus read(vfs::ReadRequest request, vfs::ReadResult *result);
        OsStatus write(vfs::WriteRequest request, vfs::WriteResult *result);
    };
}
