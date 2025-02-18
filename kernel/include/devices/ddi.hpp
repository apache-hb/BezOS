#pragma once

#include <bezos/subsystem/ddi.h>

#include "fs2/device.hpp"
#include "display.hpp"

namespace dev {
    class DisplayHandle : public vfs2::IVfsNodeHandle {
        km::Canvas mCanvas;

    public:
        DisplayHandle(vfs2::IVfsNode *node, km::Canvas canvas)
            : vfs2::IVfsNodeHandle(node)
            , mCanvas(canvas)
        { }
    };

    class DisplayDevice : public vfs2::IVfsDevice {
        km::Canvas mCanvas;

        // vfs2::IVfsDevice interface
        OsStatus query(sm::uuid uuid, vfs2::IVfsNodeHandle **handle) override {
            if (uuid != kOsDisplayClassGuid) {
                return OsStatusNotSupported;
            }

            DisplayHandle *ddiHandle = new (std::nothrow) DisplayHandle(this, mCanvas);
            if (!ddiHandle) {
                return OsStatusOutOfMemory;
            }

            *handle = ddiHandle;
            return OsStatusSuccess;
        }

    public:
        DisplayDevice(km::Canvas canvas)
            : mCanvas(canvas)
        { }
    };
}
