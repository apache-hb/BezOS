#pragma once

#include <bezos/subsystem/identify.h>

#include "fs2/node.hpp"

namespace vfs2 {
    class VfsIdentifyHandle : public IVfsNodeHandle {
        OsStatus serveInterfaceList(void *data, size_t size);
        OsStatus serveInfo(void *data, size_t size);

    public:
        VfsIdentifyHandle(IVfsNode *node)
            : IVfsNodeHandle(node)
        { }

        OsStatus invoke(uint64_t function, void *data, size_t size) override;
    };
}
