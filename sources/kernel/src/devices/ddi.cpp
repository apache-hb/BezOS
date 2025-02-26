#include "devices/ddi.hpp"
#include "panic.hpp"

dev::DisplayHandle::DisplayHandle(vfs2::IVfsDevice *node)
    : vfs2::IVfsNodeHandle(node)
    , mCanvas(static_cast<DisplayDevice*>(node)->getCanvas())
{
    km::MemoryRange range = {
        .front = mCanvas.physical(),
        .back = mCanvas.physical() + mCanvas.size() * mCanvas.bytesPerPixel(),
    };

    auto& pm = km::GetProcessPageManager();
    if (OsStatus status = pm.map(range, km::PageFlags::eUser | km::PageFlags::eWrite, km::MemoryType::eWriteCombine, &mUserCanvas)) {
        KmDebugMessage("[DDI] Failed to map display canvas: ", status, "\n");
        KM_PANIC("Failed to map display canvas.");
    }
}
