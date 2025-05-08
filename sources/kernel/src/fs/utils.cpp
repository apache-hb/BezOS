#include "fs/utils.hpp"

#include "fs/base.hpp"
#include "fs/interface.hpp"

bool vfs::HasInterface(sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data, size_t size) {
    std::unique_ptr<IHandle> handle;
    if (node->query(uuid, data, size, std::out_ptr(handle)) != OsStatusSuccess) {
        return false;
    }

    return true;
}

OsStatus vfs::OpenFileInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IFileHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsFileGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IFileHandle*>(result.release());
    return OsStatusSuccess;
}

OsStatus vfs::OpenFolderInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IFolderHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsFolderGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IFolderHandle*>(result.release());
    return OsStatusSuccess;
}

OsStatus vfs::OpenIteratorInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IIteratorHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsIteratorGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IIteratorHandle*>(result.release());
    return OsStatusSuccess;
}

OsStatus vfs::OpenIdentifyInterface(sm::RcuSharedPtr<INode> node, const void *data, size_t size, IIdentifyHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = node->query(kOsIdentifyGuid, data, size, std::out_ptr(result))) {
        return status;
    }

    *handle = static_cast<IIdentifyHandle*>(result.release());
    return OsStatusSuccess;
}

sm::RcuSharedPtr<vfs::INode> vfs::GetParentNode(sm::RcuSharedPtr<INode> node) {
    NodeInfo info = node->info();
    return info.parent.lock();
}

sm::RcuSharedPtr<vfs::INode> vfs::GetParentNode(IHandle *handle) {
    return GetParentNode(GetHandleNode(handle));
}

sm::RcuSharedPtr<vfs::INode> vfs::GetHandleNode(IHandle *handle) {
    HandleInfo info = handle->info();
    return info.node;
}

vfs::IVfsMount *vfs::GetMount(sm::RcuSharedPtr<INode> node) {
    NodeInfo info = node->info();
    return info.mount;
}

vfs::IVfsMount *vfs::GetMount(IHandle *handle) {
    return GetMount(GetHandleNode(handle));
}
