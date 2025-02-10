#include "fs2/ramfs.hpp"

using namespace vfs2;

//
// ramfs file implementation
//

OsStatus RamFsFile::read(ReadRequest request, ReadResult *result) {
    uintptr_t range = (uintptr_t)request.end - (uintptr_t)request.begin;
    size_t size = std::min<size_t>(range, mData.count() - request.offset);
    memcpy(request.begin, mData.data() + request.offset, size);
    result->read = size;
    return OsStatusSuccess;
}

OsStatus RamFsFile::write(WriteRequest request, WriteResult *result) {
    uintptr_t range = (uintptr_t)request.end - (uintptr_t)request.begin;
    if ((request.offset + range) > mData.count()) {
        mData.resize(request.offset + range);
    }

    memcpy(mData.data() + request.offset, request.begin, range);
    result->write = range;
    return OsStatusSuccess;
}

//
// ramfs folder implementation
//

OsStatus RamFsFolder::create(IVfsNode **node) {
    IVfsNode *file = new(std::nothrow) RamFsFile();
    if (!file) {
        return OsStatusOutOfMemory;
    }

    *node = file;
    return OsStatusSuccess;
}

OsStatus RamFsFolder::remove(IVfsNode *node) {
    auto it = children.find(node->name);
    if (it == children.end()) {
        return OsStatusNotFound;
    }

    children.erase(it);
    delete node;

    return OsStatusSuccess;
}

OsStatus RamFsFolder::rmdir(IVfsNode* node) {
    auto it = children.find(node->name);
    if (it == children.end()) {
        return OsStatusNotFound;
    }

    children.erase(it);
    delete node;

    return OsStatusSuccess;
}

OsStatus RamFsFolder::mkdir(IVfsNode **node) {
    IVfsNode *folder = new(std::nothrow) RamFsFolder();
    if (!folder) {
        return OsStatusOutOfMemory;
    }

    *node = folder;
    return OsStatusSuccess;
}

//
// ramfs mount implementation
//

OsStatus RamFsMount::root(IVfsNode **node) {
    *node = mRootNode;
    return OsStatusSuccess;
}

//
// ramfs implementation
//

OsStatus RamFs::mount(IVfsMount **mount) {
    *mount = new RamFsMount(this);
    return OsStatusSuccess;
}

OsStatus RamFs::unmount(IVfsMount *mount) {
    delete mount;
    return OsStatusSuccess;
}

RamFsMount::RamFsMount(RamFs *fs)
    : IVfsMount(fs)
    , mRootNode(new RamFsFolder())
{
    mRootNode->mount = this;
    mRootNode->type = VfsNodeType::eFolder;
}

RamFs& RamFs::instance() {
    static RamFs sRamFs{};
    return sRamFs;
}
