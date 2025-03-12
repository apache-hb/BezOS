#include "fs2/vfs.hpp"
#include "fs2/node.hpp"
#include "fs2/ramfs.hpp"

#include "log.hpp"

using namespace vfs2;

VfsRoot::VfsRoot() {
    // TODO: This isn't conducive to good error handling.
    //       Need to find a way of propagating errors up, maybe enable exceptions?
    if (OsStatus status = RamFs::instance().mount(std::out_ptr(mRootMount))) {
        KmDebugMessage("Failed to mount root filesystem: ", unsigned(status), "\n");
        KM_PANIC("Failed to mount root filesystem.");
    }

    if (OsStatus status = mRootMount->root(std::out_ptr(mRootNode))) {
        KmDebugMessage("Failed to get root node: ", unsigned(status), "\n");
        KM_PANIC("Failed to get root node.");
    }
}

OsStatus VfsRoot::walk(const VfsPath& path, INode **parent) {
    //
    // If the path is only one segment long then the parent
    // is the root node.
    //
    if (path.segmentCount() == 1) {
        *parent = mRootNode.get();
        return OsStatusSuccess;
    }

    //
    // Otherwise we need to walk the path to find the parent
    // which is deferred to lookup.
    //
    return lookupUnlocked(path.parent(), parent);
}

OsStatus VfsRoot::lookupUnlocked(const VfsPath& path, INode **node) {
    //
    // This function is expected to be called with the lock
    // held. It is the responsibility of the caller to ensure
    // that the lock is held.
    //
    INode *current = mRootNode.get();

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

OsStatus VfsRoot::insertMount(IVfsNode *parent, const VfsPath& path, std::unique_ptr<IVfsMount> object, IVfsMount **mount) {
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

    INode *root = nullptr;
    if (OsStatus status = point->root(&root)) {
        mMounts.erase(iter);
        return status;
    }

    //
    // Fill out the root node with its information relative
    // to where its mounted.
    //

    root->name = VfsString(path.name());
    root->parent = parent;

    if (OsStatus status = parent->addNode(path.name(), root)) {
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
    stdx::UniqueLock guard(mLock);

    //
    // Mount points follow the same rules as other vfs objects
    // and must therefore have a parent directory.
    //
    INode *parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    //
    // Create the new mount point object.
    //
    std::unique_ptr<IVfsMount> impl;
    if (OsStatus status = driver->mount(std::out_ptr(impl))) {
        return status;
    }

    return insertMount(parent, path, std::move(impl), mount);
}

OsStatus VfsRoot::create(const VfsPath& path, IVfsNode **node) {
    stdx::UniqueLock guard(mLock);

    //
    // Walk along the fs to find the parent folder
    // to the requested path.
    //
    INode *parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    //
    // Create the new file inside the parent folder.
    //
    INode *child = nullptr;
    if (OsStatus status = parent->addFile(path.name(), &child)) {
        return status;
    }

    //
    // Return the newly created file up to the caller.
    //
    *node = child;
    return OsStatusSuccess;
}

OsStatus VfsRoot::remove(INode *node) {
    //
    // remove is only valid on files, each inode type
    // has its own method for removal.
    //
    if (!node->isA(kOsFileGuid)) {
        return OsStatusInvalidType;
    }

    //
    // If the node has no parent, we are unable to remove it.
    // Additionally this is a contract violation as all nodes
    // aside from the root node must have a parent.
    //
    if (node->parent == nullptr) {
        return OsStatusInvalidInput;
    }

    //
    // Ensure that no programs hold exclusive locks on the file.
    //
    if (!node->readyForRemove()) {
        return OsStatusHandleLocked;
    }

    stdx::UniqueLock guard(mLock);

    //
    // Once we know the node is ready for removal we can
    // remove it from the parent folder.
    //
    if (OsStatus status = node->parent->remove(node)) {
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

OsStatus VfsRoot::mkdir(const VfsPath& path, INode **node) {
    stdx::UniqueLock guard(mLock);

    INode *parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    INode *child = nullptr;
    if (OsStatus status = parent->addFolder(path.name(), &child)) {
        return status;
    }

    *node = child;
    return OsStatusSuccess;
}

OsStatus VfsRoot::rmdir(INode *node) {

    //
    // rmdir is only valid on folders, each inode type
    // has its own method for removal.
    //
    if (!node->isA(kOsFolderGuid)) {
        return OsStatusInvalidType;
    }

    //
    // If the node has no parent, we are unable to remove it.
    // This indicates an attempt to remove the fs root node.
    //
    if (node->parent == nullptr) {
        return OsStatusInvalidInput;
    }

    //
    // Ensure that no programs hold exclusive locks on the folder.
    //
    if (!node->readyForRemove()) {
        return OsStatusHandleLocked;
    }

    stdx::UniqueLock guard(mLock);

    //
    // Once we know the node is ready for removal we can
    // remove it from the parent folder.
    //
    if (OsStatus status = node->parent->rmdir(node)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus VfsRoot::mkpath(const VfsPath& path, INode **node) {
    stdx::UniqueLock guard(mLock);

    INode *current = mRootNode.get();

    for (auto segment : path) {
        INode *child = nullptr;
        OsStatus lookupStatus = current->lookup(segment, &child);

        if (lookupStatus == OsStatusSuccess) {
            //
            // If there is an inode then we must ensure it's
            // a folder before continuing traversal.
            //
            if (!child->isA(kOsFolderGuid)) {
                return OsStatusTraverseNonFolder;
            }
        } else if (lookupStatus == OsStatusNotFound) {
            //
            // If there is no inode with this name then
            // we need to create a folder.
            //

            if (OsStatus status = current->addFolder(segment, &child)) {
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

OsStatus VfsRoot::lookup(const VfsPath& path, INode **node) {
    stdx::SharedLock guard(mLock);

    return lookupUnlocked(path, node);
}

OsStatus VfsRoot::mkdevice(const VfsPath& path, INode *device) {
    stdx::UniqueLock guard(mLock);

    INode *parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    //
    // We need to initialize the device node before adding it to the parent.
    //
    parent->initNode(device, path.name(), VfsNodeType::eNone);

    //
    // If this call fails it is the responsibility of the caller to clean up the device.
    //
    return parent->addNode(path.name(), device);
}

OsStatus VfsRoot::device(const VfsPath& path, sm::uuid interface, const void *data, size_t size, IHandle **handle) {
    stdx::UniqueLock guard(mLock);

    //
    // If the path is empty then we are querying the root node.
    //
    if (path.segmentCount() == 0) {
        return mRootNode->query(interface, data, size, handle);
    }

    IVfsNode *parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    IVfsNode *device = nullptr;
    if (OsStatus status = parent->lookup(path.name(), &device)) {
        return status;
    }

    //
    // All vfs nodes are devices, so there is no need to check before querying for an interface.
    //

    return device->query(interface, data, size, handle);
}
