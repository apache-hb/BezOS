#pragma once

#include <bezos/subsystem/ddi.h>

#include "user/user.hpp"
#include "fs2/device.hpp"
#include "display.hpp"

#include "kernel.hpp"

namespace dev {
    class DisplayHandle : public vfs2::IVfsNodeHandle {
        km::Canvas mCanvas;
        void *mUserCanvas = nullptr;

        OsStatus blit(void *data) {
            OsDdiBlit request{};
            if (OsStatus status = km::CopyUserMemory(km::GetSystemMemory()->pt, (uintptr_t)data, sizeof(request), &request)) {
                return status;
            }

            return km::ReadUserMemory(km::GetSystemMemory()->pt, request.SourceFront, request.SourceBack, mCanvas.address(), mCanvas.size() * mCanvas.bytesPerPixel());
        }

        OsStatus info(void *data) const {
            OsDdiDisplayInfo info {
                .Name = "RAMFB",
                .Width = uint32_t(mCanvas.width()),
                .Height = uint32_t(mCanvas.height()),
                .Stride = uint32_t(mCanvas.stride()),
                .BitsPerPixel = uint8_t(mCanvas.bpp()),
                .RedMaskSize = uint8_t(mCanvas.redMaskSize()),
                .RedMaskShift = uint8_t(mCanvas.redMaskShift()),
                .GreenMaskSize = uint8_t(mCanvas.greenMaskSize()),
                .GreenMaskShift = uint8_t(mCanvas.greenMaskShift()),
                .BlueMaskSize = uint8_t(mCanvas.blueMaskSize()),
                .BlueMaskShift = uint8_t(mCanvas.blueMaskShift()),
            };

            return km::WriteUserMemory(km::GetSystemMemory()->pt, data, &info, sizeof(info));
        }

        OsStatus fill(void *data) {
            OsDdiFill request{};
            if (OsStatus status = km::CopyUserMemory(km::GetSystemMemory()->pt, (uintptr_t)data, sizeof(request), &request)) {
                return status;
            }

            km::Pixel pixel = { request.R, request.G, request.B };
            mCanvas.fill(pixel);
            return OsStatusSuccess;
        }

        OsStatus getCanvas(void *data) {
            OsDdiGetCanvas result {
                .Canvas = mUserCanvas,
            };

            return km::WriteUserMemory(km::GetSystemMemory()->pt, data, &result, sizeof(result));
        }

    public:
        DisplayHandle(vfs2::IVfsNode *node, km::Canvas canvas)
            : vfs2::IVfsNodeHandle(node)
            , mCanvas(canvas)
        {
            km::MemoryRange range = {
                .front = mCanvas.physical(),
                .back = mCanvas.physical() + mCanvas.size() * mCanvas.bytesPerPixel(),
            };

            mUserCanvas = km::GetSystemMemory()->map(range, km::PageFlags::eUser | km::PageFlags::eWrite, km::MemoryType::eWriteCombine);
        }

        OsStatus call(uint64_t function, void *data) override {
            switch (function) {
            case eOsDdiBlit:
                return blit(data);
            case eOsDdiInfo:
                return info(data);
            case eOsDdiFill:
                return fill(data);
            case eOsDdiGetCanvas:
                return getCanvas(data);

            default:
                return OsStatusInvalidFunction;
            }
        }
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
