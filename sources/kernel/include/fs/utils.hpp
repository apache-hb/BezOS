#pragma once

#include "std/rcuptr.hpp"
#include "util/uuid.hpp"

namespace vfs {
    class INode;
    class IHandle;

    class IFileHandle;
    class IFolderHandle;
    class IIteratorHandle;
    class IIdentifyHandle;

    class IVfsMount;

    bool HasInterface(sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data = nullptr, size_t size = 0);
    OsStatus OpenFileInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IFileHandle **handle);
    OsStatus OpenFolderInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IFolderHandle **handle);
    OsStatus OpenIteratorInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IIteratorHandle **handle);
    OsStatus OpenIdentifyInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IIdentifyHandle **handle);

    sm::RcuSharedPtr<INode> GetParentNode(sm::RcuSharedPtr<INode> node);
    sm::RcuSharedPtr<INode> GetParentNode(IHandle *handle);
    sm::RcuSharedPtr<INode> GetHandleNode(IHandle *handle);
    IVfsMount *GetMount(sm::RcuSharedPtr<INode> node);
    IVfsMount *GetMount(IHandle *handle);
}
