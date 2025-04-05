#include "fs2/ramfs.hpp"
#include "fs2/file.hpp"
#include "fs2/identify.hpp"
#include "fs2/iterator.hpp"
#include "fs2/utils.hpp"

using namespace vfs2;

//
// ramfs file implementation
//

static constexpr inline InterfaceList kFileInterfaceList = std::to_array({
    InterfaceOf<TIdentifyHandle<RamFsFile>, RamFsFile>(kOsIdentifyGuid),
    InterfaceOf<TFileHandle<RamFsFile>, RamFsFile>(kOsFileGuid),
});

OsStatus RamFsFile::query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) {
    return kFileInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus RamFsFile::interfaces(OsIdentifyInterfaceList *list) {
    return kFileInterfaceList.list(list);
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

static constexpr inline InterfaceList kFolderInterfaceList = std::to_array({
    InterfaceOf<TIdentifyHandle<RamFsFolder>, RamFsFolder>(kOsIdentifyGuid),
    InterfaceOf<TFolderHandle<RamFsFolder>, RamFsFolder>(kOsFolderGuid),
    InterfaceOf<TIteratorHandle<RamFsFolder>, RamFsFolder>(kOsIteratorGuid),
});

OsStatus RamFsFolder::query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) {
    return kFolderInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus RamFsFolder::interfaces(OsIdentifyInterfaceList *list) {
    return kFolderInterfaceList.list(list);
}

//
// ramfs mount implementation
//

OsStatus RamFsMount::mkdir(sm::RcuSharedPtr<INode> parent, VfsStringView name, const void *, size_t, sm::RcuSharedPtr<INode> *node) {
    std::unique_ptr<IFolderHandle> folder;
    if (OsStatus status = OpenFolderInterface(parent, nullptr, 0, std::out_ptr(folder))) {
        return status;
    }

    sm::RcuSharedPtr<RamFsFolder> child = sm::rcuMakeShared<RamFsFolder>(mDomain, parent, this, VfsString(name));
    if (!child) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = folder->mknode(name, child)) {
        return status;
    }

    *node = child;
    return OsStatusSuccess;
}

OsStatus RamFsMount::create(sm::RcuSharedPtr<INode> parent, VfsStringView name, const void *, size_t, sm::RcuSharedPtr<INode> *node) {
    std::unique_ptr<IFolderHandle> folder;
    if (OsStatus status = OpenFolderInterface(parent, nullptr, 0, std::out_ptr(folder))) {
        return status;
    }

    sm::RcuSharedPtr<RamFsFile> file = sm::rcuMakeShared<RamFsFile>(mDomain, parent, this, VfsString(name));
    if (!file) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = folder->mknode(name, file)) {
        return status;
    }

    *node = file;
    return OsStatusSuccess;
}

OsStatus RamFsMount::root(sm::RcuSharedPtr<INode> *node) {
    *node = mRootNode;
    return OsStatusSuccess;
}

//
// ramfs driver implementation
//

OsStatus RamFs::mount(sm::RcuDomain *domain, IVfsMount **mount) {
    RamFsMount *node = new(std::nothrow) RamFsMount(this, domain);
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

RamFsMount::RamFsMount(RamFs *fs, sm::RcuDomain *domain)
    : IVfsMount(fs, domain)
    , mRootNode(sm::rcuMakeShared<RamFsFolder>(mDomain, nullptr, this, ""))
{ }

RamFs& RamFs::instance() {
    static RamFs sDriver{};
    return sDriver;
}
