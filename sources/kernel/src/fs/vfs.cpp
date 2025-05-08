#include "fs/vfs.hpp"
#include "fs/node.hpp"
#include "fs/ramfs.hpp"

#include "fs/utils.hpp"
#include "log.hpp"

using namespace vfs;

VfsRoot::VfsRoot() {
    // TODO: This isn't conducive to good error handling.
    //       Need to find a way of propagating errors up, maybe enable exceptions?
    if (OsStatus status = RamFs::instance().mount(&mDomain, std::out_ptr(mRootMount))) {
        KmDebugMessage("Failed to mount root filesystem: ", unsigned(status), "\n");
        KM_PANIC("Failed to mount root filesystem.");
    }

    if (OsStatus status = mRootMount->root(&mRootNode)) {
        KmDebugMessage("Failed to get root node: ", unsigned(status), "\n");
        KM_PANIC("Failed to get root node.");
    }
}

OsStatus VfsRoot::walk(const VfsPath& path, sm::RcuSharedPtr<INode> *parent) {
    //
    // If the path is only one segment long then the parent
    // is the root node.
    //
    if (path.segmentCount() == 1) {
        *parent = mRootNode;
        return OsStatusSuccess;
    }

    //
    // Otherwise we need to walk the path to find the parent
    // which is deferred to lookup.
    //
    return lookupUnlocked(path.parent(), parent);
}

OsStatus VfsRoot::lookupUnlocked(const VfsPath& path, sm::RcuSharedPtr<INode> *node) {
    //
    // This function is expected to be called with the lock
    // held. It is the responsibility of the caller to ensure
    // that the lock is held.
    //
    sm::RcuSharedPtr<INode> current = mRootNode;

    for (auto segment : path) {
        std::unique_ptr<IHandle> folder;
        if (OsStatus status = current->query(kOsFolderGuid, nullptr, 0, std::out_ptr(folder))) {
            //
            // If this node is not a folder then the path is malformed.
            //
            if (status == OsStatusNotSupported) {
                return OsStatusTraverseNonFolder;
            }

            return status;
        }

        IFolderHandle *handle = static_cast<IFolderHandle*>(folder.get());

        //
        // Search the current folder for the segment name
        // we are looking for.
        //
        if (OsStatus status = handle->lookup(segment, &current)) {
            return status;
        }
    }

    *node = current;
    return OsStatusSuccess;
}

OsStatus VfsRoot::createFolder(IFolderHandle *folder, VfsString name, sm::RcuSharedPtr<INode> *node) {
    sm::RcuSharedPtr<INode> parent = GetHandleNode(folder);
    IVfsMount *mount = GetMount(folder);

    sm::RcuSharedPtr<INode> child;
    if (OsStatus status = mount->mkdir(parent, name, nullptr, 0, &child)) {
        return status;
    }

    *node = child;
    return OsStatusSuccess;
}

OsStatus VfsRoot::addNode(sm::RcuSharedPtr<INode> parent, VfsString name, sm::RcuSharedPtr<INode> child) {
    std::unique_ptr<IFolderHandle> handle;
    if (OsStatus status = queryFolder(parent, std::out_ptr(handle))) {
        return status;
    }

    return handle->mknode(name, child);
}

OsStatus VfsRoot::queryFolder(sm::RcuSharedPtr<INode> parent, IFolderHandle **handle) {
    std::unique_ptr<IHandle> result;
    if (OsStatus status = parent->query(kOsFolderGuid, nullptr, 0, std::out_ptr(result))) {
        return status;
    }

    //
    // All folders must inherit from the folder handle interface, so this cast is ok.
    //
    *handle = static_cast<IFolderHandle*>(result.release());
    return OsStatusSuccess;
}

bool VfsRoot::hasInterface(sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data, size_t size) {
    std::unique_ptr<IHandle> handle;
    if (node->query(uuid, data, size, std::out_ptr(handle)) != OsStatusSuccess) {
        return false;
    }

    return true;
}

OsStatus VfsRoot::insertMount(sm::RcuSharedPtr<INode> parent, const VfsPath& path, std::unique_ptr<IVfsMount> object, IVfsMount **mount) {
    stdx::UniqueLock guard(mLock);

    //
    // Add the mount point to our internal mappings.
    //
    auto [iter, ok] = mMounts.insert({ path, std::move(object) });
    if (!ok) {
        //
        // If the mount point wasn't added it must already exist.
        //
        return OsStatusAlreadyExists;
    }

    //
    // Take a reference to the newly created mount object so
    // it can be added to the parent folder, and then returned
    // to the caller.
    //
    auto& [_, result] = *iter;
    IVfsMount *point = result.get();

    //
    // Add the new mount points root node to the parent folder
    // to allow us to traverse the newly mounted fs. If either
    // of these operations fail for any reason then we need to
    // unregister the mount as it would be inaccessible without
    // its root node present in its parent folder.
    //

    sm::RcuSharedPtr<INode> mountBase = nullptr;
    if (OsStatus status = point->root(&mountBase)) {
        mMounts.erase(iter);
        return status;
    }

    //
    // Fill out the root node with its information relative
    // to where its mounted.
    //
    mountBase->init(parent, VfsString(path.name()), sys::NodeAccess::RWX);

    if (OsStatus status = addNode(parent, VfsString(path.name()), mountBase)) {
        mMounts.erase(iter);
        return status;
    }

    //
    // Return the created mount object to the caller.
    //
    *mount = result.get();

    return OsStatusSuccess;
}

OsStatus VfsRoot::addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount) {
    //
    // Mount points follow the same rules as other vfs objects
    // and must therefore have a parent directory.
    //
    sm::RcuSharedPtr<INode> parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    //
    // Create the new mount point object.
    //
    std::unique_ptr<IVfsMount> impl;
    if (OsStatus status = driver->mount(&mDomain, std::out_ptr(impl))) {
        return status;
    }

    return insertMount(parent, path, std::move(impl), mount);
}

OsStatus VfsRoot::create(const VfsPath& path, sm::RcuSharedPtr<INode> *node) {
    sm::RcuGuard rcuGuard(mDomain);

    //
    // Walk along the fs to find the parent folder
    // to the requested path.
    //
    sm::RcuSharedPtr<INode> parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    NodeInfo cInfo = parent->info();
    IVfsMount *mount = cInfo.mount;

    //
    // Create the new file inside the parent folder.
    //
    sm::RcuSharedPtr<INode> child = nullptr;
    if (OsStatus status = mount->create(parent, path.name(), nullptr, 0, &child)) {
        return status;
    }

    //
    // Return the newly created file up to the caller.
    //
    *node = child;
    return OsStatusSuccess;
}

OsStatus VfsRoot::remove(sm::RcuSharedPtr<INode> node) {
    NodeInfo nInfo = node->info();
    sm::RcuSharedPtr<INode> parent = nInfo.parent.lock();
    KM_CHECK(parent != nullptr, "Node has no parent");

    //
    // remove is only valid on files, each inode type
    // has its own method for removal.
    //
    if (!hasInterface(node, kOsFileGuid)) {
        return OsStatusInvalidType;
    }

    //
    // If the node has no parent, we are unable to remove it.
    // Additionally this is a contract violation as all nodes
    // aside from the root node must have a parent.
    //
    if (parent == nullptr) {
        return OsStatusInvalidInput;
    }

    std::unique_ptr<IFolderHandle> folder;
    if (OsStatus status = queryFolder(parent, std::out_ptr(folder))) {
        return status;
    }

    //
    // Once we know the node is ready for removal we can
    // remove it from the parent folder.
    //
    if (OsStatus status = folder->rmnode(node)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus VfsRoot::open(const VfsPath& path, IFileHandle **handle) {
    //
    // Any vnode that satisfies the file guid must derived from IFileHandle.
    //
    IHandle *result = nullptr;
    if (OsStatus status = device(path, kOsFileGuid, nullptr, 0, &result)) {
        return status;
    }

    *handle = static_cast<IFileHandle*>(result);
    return OsStatusSuccess;
}

OsStatus VfsRoot::opendir(const VfsPath& path, IHandle **handle) {
    return device(path, kOsFolderGuid, nullptr, 0, handle);
}

OsStatus VfsRoot::mkdir(const VfsPath& path, sm::RcuSharedPtr<INode> *node) {
    sm::RcuSharedPtr<INode> parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    std::unique_ptr<IFolderHandle> folder;
    if (OsStatus status = queryFolder(parent, std::out_ptr(folder))) {
        return status;
    }

    sm::RcuSharedPtr<INode> child = nullptr;
    if (OsStatus status = createFolder(folder.get(), VfsString(path.name()), &child)) {
        return status;
    }

    *node = child;
    return OsStatusSuccess;
}

OsStatus VfsRoot::rmdir(sm::RcuSharedPtr<INode> node) {
    NodeInfo nInfo = node->info();
    sm::RcuSharedPtr<INode> parent = nInfo.parent.lock();
    KM_CHECK(parent != nullptr, "Node has no parent");

    //
    // rmdir is only valid on folders, each inode type
    // has its own method for removal.
    //
    if (!hasInterface(node, kOsFolderGuid)) {
        return OsStatusInvalidType;
    }

    //
    // If the node has no parent, we are unable to remove it.
    // This indicates an attempt to remove the fs root node.
    //
    if (parent == nullptr) {
        return OsStatusInvalidInput;
    }

    std::unique_ptr<IFolderHandle> folder;
    if (OsStatus status = queryFolder(parent, std::out_ptr(folder))) {
        return status;
    }

    //
    // Once we know the node is ready for removal we can
    // remove it from the parent folder.
    //
    if (OsStatus status = folder->rmnode(node)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus VfsRoot::mkpath(const VfsPath& path, sm::RcuSharedPtr<INode> *node) {
    sm::RcuSharedPtr<INode> current = mRootNode;

    for (auto segment : path) {
        std::unique_ptr<IFolderHandle> folder;
        if (OsStatus status = OpenFolderInterface(current, nullptr, 0, std::out_ptr(folder)))  {
            //
            // If this node is not a folder then the path is malformed.
            //
            if (status == OsStatusInterfaceNotSupported) {
                return OsStatusTraverseNonFolder;
            }

            return status;
        }

        sm::RcuSharedPtr<INode> child = nullptr;
        OsStatus lookupStatus = folder->lookup(segment, &child);

        if (lookupStatus == OsStatusSuccess) {
            //
            // We can keep walking the path. This if block is left
            // intentionally empty.
            //
        } else if (lookupStatus == OsStatusNotFound) {
            //
            // If there is no inode with this name then
            // we need to create a folder.
            //
            if (OsStatus status = createFolder(folder.get(), VfsString(segment), &child)) {
                return status;
            }

        } else {
            //
            // If we got an error we can't handle then propogate it
            // up to the caller.
            //

            return lookupStatus;
        }

        current = child;
    }

    *node = current;
    return OsStatusSuccess;
}

OsStatus VfsRoot::lookup(const VfsPath& path, sm::RcuSharedPtr<INode> *node) {
    return lookupUnlocked(path, node);
}

OsStatus VfsRoot::mkdevice(const VfsPath& path, sm::RcuSharedPtr<INode> device) {
    sm::RcuSharedPtr<INode> parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    std::unique_ptr<IFolderHandle> folder;
    if (OsStatus status = queryFolder(parent, std::out_ptr(folder))) {
        return status;
    }

    //
    // We need to initialize the device node before adding it to the parent.
    //
    device->init(parent, VfsString(path.name()), sys::NodeAccess::RWX);

    //
    // If this call fails it is the responsibility of the caller to clean up the device.
    //
    return folder->mknode(path.name(), device);
}

OsStatus VfsRoot::device(const VfsPath& path, sm::uuid interface, const void *data, size_t size, IHandle **handle) {
    //
    // If the path is empty then we are querying the root node.
    //
    if (path.segmentCount() == 0) {
        return mRootNode->query(interface, data, size, handle);
    }

    sm::RcuSharedPtr<INode> parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    std::unique_ptr<IFolderHandle> folder;
    if (OsStatus status = queryFolder(parent, std::out_ptr(folder))) {
        return status;
    }

    sm::RcuSharedPtr<INode> device = nullptr;
    if (OsStatus status = folder->lookup(path.name(), &device)) {
        return status;
    }

    //
    // All vfs nodes are devices, so there is no need to check before querying for an interface.
    //

    return device->query(interface, data, size, handle);
}
