#include "fs2/node.hpp"
#include "fs2/device.hpp"
#include "log.hpp"

using namespace vfs2;

IVfsNode::~IVfsNode() {
    for (auto& [_, inode] : mChildren) {
        delete inode;
    }
}

IVfsNode::IVfsNode(VfsNodeType type)
    : mType(type)
{
    addInterface<VfsIdentifyHandle>(kOsIdentifyGuid);
}

void IVfsNode::initNode(IVfsNode *node, VfsStringView name, VfsNodeType type) {
    node->name = VfsString(name);
    node->parent = this;
    node->mType = type;
    node->mount = mount;
}

OsStatus IVfsNode::query(sm::uuid uuid, IVfsNodeHandle **handle) {
    KmDebugMessage("[VFS] Query Interface: ", uuid, "\n");

    if (auto it = mInterfaces.find(uuid); it != mInterfaces.end()) {
        KmDebugMessage("[VFS] Found Interface: ", it->first, "\n");
        return it->second(this, handle);
    }

    KmDebugMessage("[VFS] Interface not found: ", uuid, "\n");

    return OsStatusNotSupported;
}

OsStatus IVfsNode::open(IVfsNodeHandle **handle) {
    if (!isA(VfsNodeType::eFile)) {
        return OsStatusInvalidType;
    }

    IVfsNodeHandle *result = new(std::nothrow) IVfsNodeHandle(this);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus IVfsNode::opendir(IVfsNodeHandle **handle) {
    if (!isA(VfsNodeType::eFolder)) {
        return OsStatusInvalidType;
    }

    VfsFolderHandle *result = new(std::nothrow) VfsFolderHandle(this);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus IVfsNode::lookup(VfsStringView name, IVfsNode **child) {
    if (!isA(VfsNodeType::eFolder)) {
        return OsStatusInvalidType;
    }

    auto it = mChildren.find(name);
    if (it == mChildren.end()) {
        return OsStatusNotFound;
    }

    auto& [_, node] = *it;
    *child = node;
    return OsStatusSuccess;
}

OsStatus IVfsNode::addFile(VfsStringView name, IVfsNode **child) {
    if (!isA(VfsNodeType::eFolder)) {
        return OsStatusInvalidType;
    }

    //
    // Create the file object and fill out its fields.
    //

    IVfsNode *node = nullptr;
    if (OsStatus status = create(&node)) {
        return status;
    }

    initNode(node, name, VfsNodeType::eFile);

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

OsStatus IVfsNode::addFolder(VfsStringView name, IVfsNode **child) {
    if (!isA(VfsNodeType::eFolder)) {
        return OsStatusInvalidType;
    }

    IVfsNode *node = nullptr;
    if (OsStatus status = mkdir(&node)) {
        return status;
    }

    initNode(node, name, VfsNodeType::eFolder);

    if (OsStatus status = addNode(name, node)) {
        return status;
    }

    *child = node;
    return OsStatusSuccess;
}

OsStatus IVfsNode::addNode(VfsStringView name, IVfsNode *node) {
    if (!isA(VfsNodeType::eFolder)) {
        return OsStatusInvalidType;
    }

    auto [it, ok] = mChildren.insert({ VfsString(name), node });
    if (!ok) {
        return OsStatusAlreadyExists;
    }

    return OsStatusSuccess;
}
