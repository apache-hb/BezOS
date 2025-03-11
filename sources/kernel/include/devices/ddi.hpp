#pragma once

#include <bezos/subsystem/ddi.h>

#include "fs2/node.hpp"
#include "display.hpp"

namespace dev {
    class DisplayHandle : public vfs2::IVfsNodeHandle {
        km::Canvas mCanvas;
        km::AddressMapping mUserCanvas;

        OsStatus blit(void *data, size_t size) {
            if (size != sizeof(OsDdiBlit)) {
                return OsStatusInvalidInput;
            }

            OsDdiBlit request{};
            memcpy(&request, data, sizeof(request));

            void *dst = (std::byte*)mCanvas.address() + mCanvas.bytesPerPixel();
            uintptr_t canvas = (std::byte*)request.SourceBack - (std::byte*)request.SourceFront;

            memcpy(dst, request.SourceFront, canvas);

            return OsStatusSuccess;
        }

        OsStatus info(void *data, size_t size) const {
            if (size != sizeof(OsDdiDisplayInfo)) {
                return OsStatusInvalidInput;
            }

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

            memcpy(data, &info, sizeof(OsDdiDisplayInfo));
            return OsStatusSuccess;
        }

        OsStatus fill(void *data, size_t size) {
            if (size != sizeof(OsDdiFill)) {
                return OsStatusInvalidInput;
            }

            OsDdiFill request{};
            memcpy(&request, data, sizeof(request));

            km::Pixel pixel = { request.R, request.G, request.B };
            mCanvas.fill(pixel);
            return OsStatusSuccess;
        }

        OsStatus getCanvas(void *data, size_t size) {
            if (size != sizeof(OsDdiGetCanvas)) {
                return OsStatusInvalidInput;
            }

            OsDdiGetCanvas result {
                .Canvas = (void*)mUserCanvas.vaddr,
            };

            memcpy(data, &result, sizeof(result));

            return OsStatusSuccess;
        }

    public:
        DisplayHandle(vfs2::IVfsNode *node);

        OsStatus invoke(uint64_t function, void *data, size_t size) override {
            switch (function) {
            case eOsDdiBlit:
                return blit(data, size);
            case eOsDdiInfo:
                return info(data, size);
            case eOsDdiFill:
                return fill(data, size);
            case eOsDdiGetCanvas:
                return getCanvas(data, size);

            default:
                return OsStatusInvalidFunction;
            }
        }
    };

    class DisplayDevice : public vfs2::IVfsNode {
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
