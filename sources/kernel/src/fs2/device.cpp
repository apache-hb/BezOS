#include "fs2/device.hpp"
#include "kernel.hpp"

static constexpr size_t kMaxInterfaceCount = 16;

OsStatus vfs2::VfsIdentifyHandle::serveInterfaceList(void *data, size_t size) {
    uint32_t count = (size - sizeof(OsIdentifyInterfaceList)) / sizeof(OsGuid);

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
    for (auto [uuid, _] : node->mInterfaces) {
        if (i >= count) {
            i += 1;
            status = OsStatusMoreData;
            continue;
        }

        if (i >= kMaxInterfaceCount) {
            KmDebugMessage("[VFS] Device ", node->name, " has more than ", kMaxInterfaceCount, " interfaces.\n");
            break;
        }

        list.list[i++] = uuid;
    }

    list.count = i;
    memcpy(data, &list, sizeof(uint32_t) + i * sizeof(OsGuid));

    return status;
}

OsStatus vfs2::VfsIdentifyHandle::serveInfo(void *data, size_t size) {
    if (size != sizeof(OsIdentifyInfo)) {
        return OsStatusInvalidInput;
    }

    OsIdentifyInfo info = node->identity();
    memcpy(data, &info, sizeof(OsIdentifyInfo));
    return OsStatusSuccess;
}

OsStatus vfs2::VfsIdentifyHandle::invoke(uint64_t function, void *data, size_t size) {
    switch (function) {
    case eOsIdentifyInterfaceList:
        return serveInterfaceList(data, size);
    case eOsIdentifyInfo:
        return serveInfo(data, size);
    default:
        return OsStatusInvalidFunction;
    }
}
