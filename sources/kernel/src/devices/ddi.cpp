#include "devices/ddi.hpp"

dev::DisplayHandle::DisplayHandle(vfs2::IVfsDevice *node)
    : vfs2::IVfsNodeHandle(node)
    , mCanvas(static_cast<DisplayDevice*>(node)->getCanvas())
{
    km::MemoryRange range = {
        .front = mCanvas.physical(),
        .back = mCanvas.physical() + mCanvas.size() * mCanvas.bytesPerPixel(),
    };

    mUserCanvas = km::GetSystemMemory()->map(range, km::PageFlags::eUser | km::PageFlags::eWrite, km::MemoryType::eWriteCombine);
}