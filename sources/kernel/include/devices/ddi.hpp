#pragma once

#include <bezos/subsystem/ddi.h>

#include "fs2/device.hpp"
#include "fs2/identify.hpp"

#include "display.hpp"

namespace dev {
    class DisplayDevice;
    class DisplayHandle;

    static constexpr inline OsIdentifyInfo kDdiIdentifyInfo {
        .DisplayName = "Generic RAMFB Display Interface",
        .Model = "Generic RAMFB Display Interface",
        .DeviceVendor = "Generic",
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    class DisplayDevice : public vfs2::BasicNode, public vfs2::ConstIdentifyMixin<kDdiIdentifyInfo> {
        km::Canvas mCanvas;

    public:
        DisplayDevice(km::Canvas canvas)
            : mCanvas(canvas)
        { }

        km::Canvas getCanvas() const { return mCanvas; }

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
    };

    class DisplayHandle : public vfs2::BasicHandle<DisplayDevice> {
        using vfs2::BasicHandle<DisplayDevice>::mNode;
        km::AddressMapping mUserCanvas;

        OsStatus blit(void *data, size_t size);
        OsStatus info(void *data, size_t size);
        OsStatus fill(void *data, size_t size);
        OsStatus getCanvas(void *data, size_t size);

    public:
        DisplayHandle(sm::RcuSharedPtr<DisplayDevice> node, const void *data, size_t size);

        OsStatus invoke(vfs2::IInvokeContext *context, uint64_t function, void *data, size_t size) override;

        vfs2::HandleInfo info() override;
    };
}
