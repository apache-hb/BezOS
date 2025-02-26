#include "fs2/device.hpp"
#include "user/user.hpp"

static constexpr size_t kMaxInterfaceCount = 16;

vfs2::IVfsDevice::IVfsDevice() {
    addInterface<VfsIdentifyHandle>(kOsIdentifyGuid);
}

OsStatus vfs2::IVfsDevice::query(sm::uuid uuid, IVfsNodeHandle **handle) {
    if (auto it = mInterfaces.find(uuid); it != mInterfaces.end()) {
        return it->second(this, handle);
    }

    return OsStatusNotSupported;
}

OsStatus vfs2::VfsIdentifyHandle::serveInterfaceList(void *data) {
    uint32_t count{};
    if (OsStatus status = km::ReadUserMemory(km::GetProcessPageTables(), data, (char*)data + sizeof(uint32_t), &count, sizeof(uint32_t))) {
        return status;
    }

    if (count <= 0 || count > kMaxInterfaceCount) {
        return OsStatusOutOfBounds;
    }

    struct InterfaceList {
        uint32_t count;
        OsGuid list[kMaxInterfaceCount];
    };

    InterfaceList list{};

    size_t i = 0;
    OsStatus status = OsStatusSuccess;
    for (auto [uuid, _] : mDevice->mInterfaces) {
        if (i >= count) {
            i += 1;
            status = OsStatusMoreData;
            continue;
        }

        if (i >= kMaxInterfaceCount) {
            KmDebugMessage("[VFS] Device ", mDevice->name, " has more than ", kMaxInterfaceCount, " interfaces.\n");
            break;
        }

        list.list[i++] = uuid;
    }

    list.count = i;
    if (OsStatus status = km::WriteUserMemory(km::GetProcessPageTables(), data, &list, sizeof(uint32_t) + i * sizeof(OsGuid))) {
        return status;
    }

    return status;
}

OsStatus vfs2::VfsIdentifyHandle::serveInfo(void *data) {
    return km::WriteUserMemory(km::GetProcessPageTables(), data, &mDevice->mIdentifyInfo, sizeof(OsIdentifyInfo));
}

OsStatus vfs2::VfsIdentifyHandle::call(uint64_t function, void *data) {
    switch (function) {
    case eOsIdentifyInterfaceList:
        return serveInterfaceList(data);
    case eOsIdentifyInfo:
        return serveInfo(data);
    default:
        return OsStatusInvalidFunction;
    }
}

vfs2::VfsIdentifyHandle::VfsIdentifyHandle(IVfsDevice *device)
    : IVfsNodeHandle(device)
    , mDevice(device)
{ }
