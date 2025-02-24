#pragma once

#include <bezos/subsystem/identify.h>

#include "fs2/node.hpp"
#include "panic.hpp"

namespace vfs2 {
    class IVfsDevice;
    class VfsIdentifyHandle;

    using VfsCreateHandle = OsStatus(*)(IVfsDevice*, IVfsNodeHandle**);

    class VfsIdentifyHandle : public IVfsNodeHandle {
        const IVfsDevice *mDevice;

        OsStatus serveInterfaceList(void *data);
        OsStatus serveInfo(void *data);

    public:
        VfsIdentifyHandle(IVfsDevice *device);

        OsStatus call(uint64_t function, void *data) override;
    };

    class IVfsDevice : public IVfsNode {
        friend VfsIdentifyHandle;

        sm::FlatHashMap<sm::uuid, VfsCreateHandle> mInterfaces;

    protected:
        OsIdentifyInfo mIdentifyInfo;

        template<std::derived_from<IVfsNodeHandle> T>
        void addInterface(sm::uuid uuid) {
            VfsCreateHandle callback = [](IVfsDevice *device, IVfsNodeHandle **handle) -> OsStatus {
                T *objHandle = new (std::nothrow) T(device);
                if (objHandle == nullptr) {
                    return OsStatusOutOfMemory;
                }

                *handle = objHandle;
                return OsStatusSuccess;
            };

            auto [_, ok] = mInterfaces.insert({ uuid, callback });
            KM_CHECK(ok, "Failed to add interface.");
        }

        OsStatus query(sm::uuid uuid, IVfsNodeHandle **handle) override;

    public:
        IVfsDevice();
    };
}
