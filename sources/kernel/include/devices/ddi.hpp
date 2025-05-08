#pragma once

#include <bezos/subsystem/ddi.h>

#include "fs/device.hpp"
#include "fs/identify.hpp"

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

    class DisplayDevice : public vfs::BasicNode, public vfs::ConstIdentifyMixin<kDdiIdentifyInfo> {
        km::Canvas mCanvas;

    public:
        DisplayDevice(km::Canvas canvas)
            : mCanvas(canvas)
        { }

        km::Canvas getCanvas() const { return mCanvas; }

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
    };

    class DisplayHandle : public vfs::BaseHandle<DisplayDevice> {
        using vfs::BaseHandle<DisplayDevice>::mNode;
        km::AddressMapping mUserCanvas;

        OsStatus blit(void *data, size_t size);
        OsStatus info(void *data, size_t size);
        OsStatus fill(void *data, size_t size);
        OsStatus getCanvas(void *data, size_t size);

    public:
        DisplayHandle(sm::RcuSharedPtr<DisplayDevice> node, const void *data, size_t size);

        OsStatus invoke(vfs::IInvokeContext *context, uint64_t function, void *data, size_t size) override;

        vfs::HandleInfo info() override;
    };
}
