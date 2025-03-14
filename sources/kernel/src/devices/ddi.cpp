#include "devices/ddi.hpp"
#include "fs2/query.hpp"
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

OsStatus dev::DisplayHandle::invoke(vfs2::IInvokeContext *, uint64_t function, void *data, size_t size) {
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
    return vfs2::HandleInfo { mNode, kOsDisplayClassGuid };
}

static constexpr inline vfs2::InterfaceList kInterfaceList = std::to_array({
    vfs2::InterfaceOf<vfs2::TIdentifyHandle<dev::DisplayDevice>, dev::DisplayDevice>(kOsIdentifyGuid),
    vfs2::InterfaceOf<dev::DisplayHandle, dev::DisplayDevice>(kOsDisplayClassGuid),
});

OsStatus dev::DisplayDevice::query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) {
    return kInterfaceList.query(this, uuid, data, size, handle);
}

OsStatus dev::DisplayDevice::interfaces(OsIdentifyInterfaceList *list) {
    return kInterfaceList.list(list);
}
