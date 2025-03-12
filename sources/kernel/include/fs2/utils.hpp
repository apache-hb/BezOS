#pragma once

#include "util/uuid.hpp"

namespace vfs2 {
    class INode;
    class IHandle;
    class IFileHandle;
    class IFolderHandle;

    bool HasInterface(INode *node, sm::uuid uuid, const void *data = nullptr, size_t size = 0);
    OsStatus OpenFileInterface(INode *node, const void *data, size_t size, IFileHandle **handle);
    OsStatus OpenFolderInterface(INode *node, const void *data, size_t size, IFolderHandle **handle);
}
