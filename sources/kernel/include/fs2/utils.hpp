#pragma once

#include "util/uuid.hpp"

namespace vfs2 {
    class INode;
    class IHandle;

    class IFileHandle;
    class IFolderHandle;
    class IIteratorHandle;
    class IIdentifyHandle;

    struct IVfsMount;

    bool HasInterface(INode *node, sm::uuid uuid, const void *data = nullptr, size_t size = 0);
    OsStatus OpenFileInterface(INode *node, const void *data, size_t size, IFileHandle **handle);
    OsStatus OpenFolderInterface(INode *node, const void *data, size_t size, IFolderHandle **handle);
    OsStatus OpenIteratorInterface(INode *node, const void *data, size_t size, IIteratorHandle **handle);
    OsStatus OpenIdentifyInterface(INode *node, const void *data, size_t size, IIdentifyHandle **handle);

    INode *GetParentNode(INode *node);
    INode *GetParentNode(IHandle *handle);
    INode *GetHandleNode(IHandle *handle);
    IVfsMount *GetMount(INode *node);
    IVfsMount *GetMount(IHandle *handle);
}
