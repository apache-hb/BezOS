#include "fs2/utils.hpp"

#include "fs2/base.hpp"
#include "fs2/interface.hpp"

bool vfs2::HasInterface(sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data, size_t size) {
    std::unique_ptr<IHandle> handle;
    if (node->query(uuid, data, size, std::out_ptr(handle)) != OsStatusSuccess) {
        return false;
    }

    return true;
}

OsStatus vfs2::OpenFileInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IFileHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsFileGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IFileHandle*>(result.release());
    return OsStatusSuccess;
}

OsStatus vfs2::OpenFolderInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IFolderHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsFolderGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IFolderHandle*>(result.release());
    return OsStatusSuccess;
}

OsStatus vfs2::OpenIteratorInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IIteratorHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsIteratorGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IIteratorHandle*>(result.release());
    return OsStatusSuccess;
}

OsStatus vfs2::OpenIdentifyInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IIdentifyHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsIdentifyGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IIdentifyHandle*>(result.release());
    return OsStatusSuccess;
}

sm::RcuSharedPtr<vfs2::INode> vfs2::GetParentNode(sm::RcuSharedPtr<INode> node) {
    NodeInfo info = node->info();
    return info.parent.lock();
}

sm::RcuSharedPtr<vfs2::INode> vfs2::GetParentNode(IHandle *handle) {
    return GetParentNode(GetHandleNode(handle));
}

sm::RcuSharedPtr<vfs2::INode> vfs2::GetHandleNode(IHandle *handle) {
    HandleInfo info = handle->info();
    return info.node;
}

vfs2::IVfsMount *vfs2::GetMount(sm::RcuSharedPtr<INode> node) {
    NodeInfo info = node->info();
    return info.mount;
}

vfs2::IVfsMount *vfs2::GetMount(IHandle *handle) {
    return GetMount(GetHandleNode(handle));
}
