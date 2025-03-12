#include "fs2/ramfs.hpp"

using namespace vfs2;

//
// ramfs file implementation
//

OsStatus RamFsFile::query(sm::uuid uuid, const void *, size_t, IHandle **handle) {
    if (uuid == kOsFileGuid) {
        auto *file = new(std::nothrow) TFileHandle<RamFsFile>(this);
        if (!file) {
            return OsStatusOutOfMemory;
        }

        *handle = file;
        return OsStatusSuccess;
    }

    return OsStatusNotSupported;
}

OsStatus RamFsFile::read(ReadRequest request, ReadResult *result) {
    stdx::SharedLock lock(mLock);
    uintptr_t range = (uintptr_t)request.end - (uintptr_t)request.begin;
    size_t size = std::min<size_t>(range, mData.count() - request.offset);
    memcpy(request.begin, mData.data() + request.offset, size);
    result->read = size;
    return OsStatusSuccess;
}

OsStatus RamFsFile::write(WriteRequest request, WriteResult *result) {
    stdx::UniqueLock lock(mLock);
    uintptr_t range = (uintptr_t)request.end - (uintptr_t)request.begin;
    if ((request.offset + range) > mData.count()) {
        mData.resize(request.offset + range);
    }

    memcpy(mData.data() + request.offset, request.begin, range);
    result->write = range;
    return OsStatusSuccess;
}

OsStatus RamFsFile::stat(NodeStat *stat) {
    stdx::SharedLock lock(mLock);
    *stat = NodeStat {
        .logical = mData.count(),
        .blksize = 1,
        .blocks = mData.count(),
    };
    return OsStatusSuccess;
}

//
// ramfs folder implementation
//

OsStatus RamFsFolder::query(sm::uuid uuid, const void *, size_t, IHandle **handle) {
    if (uuid == kOsFolderGuid) {
        auto *folder = new(std::nothrow) TFolderHandle<RamFsFolder>(this);
        if (!folder) {
            return OsStatusOutOfMemory;
        }

        *handle = folder;
        return OsStatusSuccess;
    }

    return OsStatusNotSupported;
}

//
// ramfs mount implementation
//

OsStatus RamFsMount::mkdir(INode *parent, VfsStringView name, const void *, size_t, INode **node) {
    RamFsFolder *folder = new(std::nothrow) RamFsFolder(parent, this, VfsString(name));
    if (!folder) {
        return OsStatusOutOfMemory;
    }

    *node = folder;
    return OsStatusSuccess;
}

OsStatus RamFsMount::create(INode *parent, VfsStringView name, const void *, size_t, INode **node) {
    RamFsFile *file = new(std::nothrow) RamFsFile(parent, this, VfsString(name));
    if (!file) {
        return OsStatusOutOfMemory;
    }

    *node = file;
    return OsStatusSuccess;
}

OsStatus RamFsMount::root(INode **node) {
    *node = mRootNode;
    return OsStatusSuccess;
}

//
// ramfs driver implementation
//

OsStatus RamFs::mount(IVfsMount **mount) {
    RamFsMount *node = new(std::nothrow) RamFsMount(this);
    if (!node) {
        return OsStatusOutOfMemory;
    }

    *mount = node;
    return OsStatusSuccess;
}

OsStatus RamFs::unmount(IVfsMount *mount) {
    delete mount;
    return OsStatusSuccess;
}

RamFsMount::RamFsMount(RamFs *fs)
    : IVfsMount(fs)
    , mRootNode(new RamFsFolder(nullptr, this, ""))
{ }

RamFs& RamFs::instance() {
    static RamFs sDriver{};
    return sDriver;
}
