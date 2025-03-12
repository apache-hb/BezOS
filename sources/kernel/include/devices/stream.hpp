#pragma once

#include "fs2/device.hpp"
#include "fs2/identify.hpp"

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

    class StreamDevice : public vfs2::BasicNode, public vfs2::ConstIdentifyMixin<kStreamInfo> {
        static constexpr inline auto kInterfaceList = std::to_array({
            kOsIdentifyGuid,
            kOsStreamGuid,
        });

        std::unique_ptr<std::byte[]> mBuffer;
        stdx::SpinLock mLock;
        stdx::FixedSizeDeque<std::byte> mQueue;

    public:
        StreamDevice(size_t size);

        std::span<const OsGuid> interfaces() { return kInterfaceList; }
        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;

        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);
        OsStatus write(vfs2::WriteRequest request, vfs2::WriteResult *result);
    };
}
