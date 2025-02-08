#include "fs2/vfs.hpp"

#include "panic.hpp"

using namespace vfs2;

IVfsNode::~IVfsNode() {
    for (auto& [_, inode] : children) {
        delete inode;
    }
}

OsStatus IVfsNode::open(IVfsNodeHandle **handle) {
    KM_ASSERT(type == VfsNodeType::eFile);

    IVfsNodeHandle *result = new(std::nothrow) IVfsNodeHandle(this);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus IVfsNode::lookup(VfsStringView name, IVfsNode **child) {
    KM_ASSERT(type == VfsNodeType::eFolder);

    auto it = children.find(name);
    if (it == children.end()) {
        return OsStatusNotFound;
    }

    auto& [_, node] = *it;
    *child = node;
    return OsStatusSuccess;
}

OsStatus IVfsNode::addFile(VfsStringView name, IVfsNode **child) {
    KM_ASSERT(type == VfsNodeType::eFolder);

    //
    // Create the file object and fill out its fields.
    //

    IVfsNode *node = new(std::nothrow) IVfsNode();
    if (!node) {
        return OsStatusOutOfMemory;
    }

    node->name = VfsString(name);
    node->parent = this;
    node->type = VfsNodeType::eFile;
    node->mount = this->mount;

    //
    // Insert the newly created file into our folder
    // cache. If the insertion fails then the file already
    // exists. Note that this indicates a race condition, a folder
    // should always be externally synchronized before modification.
    //

    if (OsStatus status = addNode(name, node)) {
        return status;
    }

    *child = node;
    return OsStatusSuccess;
}

OsStatus IVfsNode::addNode(VfsStringView name, IVfsNode *node) {
    KM_ASSERT(type == VfsNodeType::eFolder);

    auto [it, ok] = children.insert({ VfsString(name), node });
    if (!ok) {
        return OsStatusAlreadyExists;
    }

    return OsStatusSuccess;
}

VfsRoot::VfsRoot()
    : mRootNode()
{
    mRootNode.type = VfsNodeType::eFolder;
}

OsStatus VfsRoot::walk(const VfsPath& path, IVfsNode **parent) {
    if (path.segmentCount() == 1) {
        *parent = &mRootNode;
        return OsStatusSuccess;
    }

    IVfsNode *current = &mRootNode;
    VfsPath journey = path.parent();

    for (auto segment : journey) {
        //
        // Search the current folder for the segment name
        // we are looking for.
        //
        IVfsNode *child = nullptr;
        if (OsStatus status = current->lookup(segment, &child)) {
            return status;
        }

        //
        // If the current node is a mount point we need to take it's
        // root node to follow.
        //

        if (child->type == VfsNodeType::eMount) {
            IVfsNode *root = nullptr;
            if (OsStatus status = child->mount->root(&root)) {
                return status;
            }

            assert(root->type == VfsNodeType::eFolder);
            current = root;
        } else if (child->type == VfsNodeType::eFolder) {
            //
            // If the current node is a folder we can continue
            // walking down the tree.
            //
            current = child;
        } else {
            //
            // If the node is anything else then the path is malformed.
            //
            return OsStatusNotFound;
        }
    }

    *parent = current;
    return OsStatusSuccess;
}

OsStatus VfsRoot::addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount) {
    //
    // Mount points follow the same rules as other vfs objects
    // and must therefore have a parent directory.
    //
    IVfsNode *parent = nullptr;
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

    //
    // Add the mount point to our internal mappings.
    //
    auto [iter, ok] = mMounts.insert({ path, std::move(impl) });
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

    IVfsNode *root = nullptr;
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

OsStatus VfsRoot::createFile(const VfsPath& path, IVfsNode **node) {
    //
    // Walk along the fs to find the parent folder
    // to the requested path.
    //
    IVfsNode *parent = nullptr;
    if (OsStatus status = walk(path, &parent)) {
        return status;
    }

    //
    // Create the new file inside the parent folder.
    //
    IVfsNode *child = nullptr;
    if (OsStatus status = parent->addFile(path.name(), &child)) {
        return status;
    }

    //
    // Return the newly created file up to the caller.
    //
    *node = child;
    return OsStatusSuccess;
}
