#include "devices/ddi.hpp"
#include "kernel.hpp"
#include "panic.hpp"

dev::DisplayHandle::DisplayHandle(DisplayDevice *node)
    : mNode(node)
{
    km::Canvas canvas = mNode->getCanvas();

    km::MemoryRange range = {
        .front = canvas.physical(),
        .back = canvas.physical() + canvas.size() * canvas.bytesPerPixel(),
    };

    auto& pm = km::GetProcessPageManager();
    if (OsStatus status = pm.map(range, km::PageFlags::eUser | km::PageFlags::eWrite, km::MemoryType::eWriteCombine, &mUserCanvas)) {
        KmDebugMessage("[DDI] Failed to map display canvas: ", status, "\n");
        KM_PANIC("Failed to map display canvas.");
    }
}

OsStatus dev::DisplayHandle::blit(void *data, size_t size) {
    if (size != sizeof(OsDdiBlit)) {
        return OsStatusInvalidInput;
    }

    km::Canvas canvas = mNode->getCanvas();

    OsDdiBlit request{};
    memcpy(&request, data, sizeof(request));

    void *dst = (std::byte*)canvas.address() + canvas.bytesPerPixel();
    uintptr_t fb = (std::byte*)request.SourceBack - (std::byte*)request.SourceFront;

    memcpy(dst, request.SourceFront, fb);

    return OsStatusSuccess;
}

OsStatus dev::DisplayHandle::info(void *data, size_t size) {
    if (size != sizeof(OsDdiDisplayInfo)) {
        return OsStatusInvalidInput;
    }

    km::Canvas canvas = mNode->getCanvas();

    OsDdiDisplayInfo info {
        .Name = "RAMFB",
        .Width = uint32_t(canvas.width()),
        .Height = uint32_t(canvas.height()),
        .Stride = uint32_t(canvas.stride()),
        .BitsPerPixel = uint8_t(canvas.bpp()),
        .RedMaskSize = uint8_t(canvas.redMaskSize()),
        .RedMaskShift = uint8_t(canvas.redMaskShift()),
        .GreenMaskSize = uint8_t(canvas.greenMaskSize()),
        .GreenMaskShift = uint8_t(canvas.greenMaskShift()),
        .BlueMaskSize = uint8_t(canvas.blueMaskSize()),
        .BlueMaskShift = uint8_t(canvas.blueMaskShift()),
    };

    memcpy(data, &info, sizeof(OsDdiDisplayInfo));
    return OsStatusSuccess;
}

OsStatus dev::DisplayHandle::fill(void *data, size_t size) {
    if (size != sizeof(OsDdiFill)) {
        return OsStatusInvalidInput;
    }

    km::Canvas canvas = mNode->getCanvas();

    OsDdiFill request{};
    memcpy(&request, data, sizeof(request));

    km::Pixel pixel = { request.R, request.G, request.B };
    canvas.fill(pixel);
    return OsStatusSuccess;
}

OsStatus dev::DisplayHandle::getCanvas(void *data, size_t size) {
    if (size != sizeof(OsDdiGetCanvas)) {
        return OsStatusInvalidInput;
    }

    OsDdiGetCanvas result {
        .Canvas = (void*)mUserCanvas.vaddr,
    };

    memcpy(data, &result, sizeof(result));

    return OsStatusSuccess;
}

OsStatus dev::DisplayHandle::invoke(uint64_t function, void *data, size_t size) {
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

vfs2::HandleInfo dev::DisplayHandle::info() {
    return vfs2::HandleInfo { mNode };
}

OsStatus dev::DisplayDevice::query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) {
    if (uuid == kOsIdentifyGuid) {
        auto *identify = new(std::nothrow) vfs2::TIdentifyHandle<DisplayDevice>(this);
        if (!identify) {
            return OsStatusOutOfMemory;
        }

        *handle = identify;
        return OsStatusSuccess;
    }

    if (uuid == kOsDisplayClassGuid) {
        auto *display = new(std::nothrow) DisplayHandle(this);
        if (!display) {
            return OsStatusOutOfMemory;
        }

        *handle = display;
        return OsStatusSuccess;
    }

    return OsStatusInterfaceNotSupported;
}
