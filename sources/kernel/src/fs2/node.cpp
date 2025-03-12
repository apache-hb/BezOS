#include "fs2/node.hpp"
#include "fs2/device.hpp"
#include "fs2/interface.hpp"

using namespace vfs2;

static OsStatus OpenCallback(IVfsNode *node, IHandle **handle) {
    return node->open(handle);
}

static OsStatus FolderCallback(IVfsNode *node, IHandle **handle) {
    return node->opendir(handle);
}

IVfsNode::~IVfsNode() {
    for (auto& [_, inode] : mChildren) {
        delete inode;
    }
}

IVfsNode::IVfsNode(VfsNodeType type)
    : mType(type)
{
    addInterface<VfsIdentifyHandle>(kOsIdentifyGuid);

    if (type == vfs2::VfsNodeType::eFile) {
        installFileInterface();
    } else if (type == vfs2::VfsNodeType::eFolder) {
        installFolderInterface();
    }
}

void IVfsNode::installFolderInterface() {
    mInterfaces.insert({ kOsFolderGuid, FolderCallback });
}

void IVfsNode::installFileInterface() {
    mInterfaces.insert({ kOsFileGuid, OpenCallback });
}

void IVfsNode::initNode(IVfsNode *node, VfsStringView name, VfsNodeType type) {
    node->name = VfsString(name);
    node->parent = this;
    node->mType = type;
    node->mount = mount;

    if (node->mType == vfs2::VfsNodeType::eFile) {
        node->installFileInterface();
    } else if (node->mType == vfs2::VfsNodeType::eFolder) {
        node->installFolderInterface();
    }
}

OsStatus IVfsNode::query(sm::uuid uuid, [[maybe_unused]] const void *data, [[maybe_unused]] size_t size, IHandle **handle) {
    if (auto it = mInterfaces.find(uuid); it != mInterfaces.end()) {
        return it->second(this, handle);
    }

    return OsStatusNotSupported;
}

OsStatus IVfsNode::open(IHandle **handle) {
    if (!isA(kOsFileGuid)) {
        return OsStatusInvalidType;
    }

    IFileHandle *result = new (std::nothrow) TFileHandle<IVfsNode>(this);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus IVfsNode::opendir(IHandle **handle) {
    if (!isA(kOsFolderGuid)) {
        return OsStatusInvalidType;
    }

    IFolderHandle *result = new(std::nothrow) TFolderHandle<IVfsNode>(this);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus IVfsNode::lookup(VfsStringView name, IVfsNode **child) {
    if (!isA(kOsFolderGuid)) {
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

OsStatus IVfsNode::lookup(VfsStringView name, INode **child) {
    IVfsNode *result = nullptr;
    if (OsStatus status = lookup(name, &result)) {
        return status;
    }

    *child = result;
    return OsStatusSuccess;
}

OsStatus IVfsNode::addFile(VfsStringView name, IVfsNode **child) {
    if (!isA(kOsFolderGuid)) {
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
    if (!isA(kOsFolderGuid)) {
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
    if (!isA(kOsFolderGuid)) {
        return OsStatusInvalidType;
    }

    auto [it, ok] = mChildren.insert({ VfsString(name), node });
    if (!ok) {
        return OsStatusAlreadyExists;
    }

    return OsStatusSuccess;
}
