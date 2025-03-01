#pragma once

#include <bezos/subsystem/ddi.h>

#include "kernel.hpp"
#include "user/user.hpp"
#include "fs2/device.hpp"
#include "display.hpp"

namespace dev {
    class DisplayHandle : public vfs2::IVfsNodeHandle {
        km::Canvas mCanvas;
        km::AddressMapping mUserCanvas;

        OsStatus blit(void *data) {
            OsDdiBlit request{};
            if (OsStatus status = km::CopyUserMemory(km::GetProcessPageTables(), (uintptr_t)data, sizeof(request), &request)) {
                return status;
            }

            return km::ReadUserMemory(km::GetProcessPageTables(), request.SourceFront, request.SourceBack, mCanvas.address(), mCanvas.size() * mCanvas.bytesPerPixel());
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

            return km::WriteUserMemory(km::GetProcessPageTables(), data, &info, sizeof(info));
        }

        OsStatus fill(void *data) {
            OsDdiFill request{};
            if (OsStatus status = km::CopyUserMemory(km::GetProcessPageTables(), (uintptr_t)data, sizeof(request), &request)) {
                return status;
            }

            km::Pixel pixel = { request.R, request.G, request.B };
            mCanvas.fill(pixel);
            return OsStatusSuccess;
        }

        OsStatus getCanvas(void *data) {
            OsDdiGetCanvas result {
                .Canvas = (void*)mUserCanvas.vaddr,
            };

            return km::WriteUserMemory(km::GetProcessPageTables(), data, &result, sizeof(result));
        }

    public:
        DisplayHandle(vfs2::IVfsDevice *node);

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

    public:
        DisplayDevice(km::Canvas canvas)
            : mCanvas(canvas)
        {
            addInterface<DisplayHandle>(kOsDisplayClassGuid);

            mIdentifyInfo = OsIdentifyInfo {
                .DisplayName = "Generic RAM framebuffer display",
                .Model = "Generic RAM framebuffer display",
                .DeviceVendor = "Generic",
                .DriverVendor = "BezOS",
                .DriverVersion = OS_VERSION(1, 0, 0),
            };
        }

        km::Canvas getCanvas() const { return mCanvas; }
    };
}
