#include "fs/ramfs.hpp"

static constinit vfs::RamFs gRamFs{};

vfs::RamFsNode::RamFsNode(RamFsMount *mount)
    : mMount(mount)
{ }

vfs::IFileSystemMount *vfs::RamFsNode::owner() const {
    return mMount;
}

vfs::RamFsFile::RamFsFile(RamFsMount *mount)
    : RamFsNode(mount)
{ }

OsStatus vfs::RamFsFile::read(ReadRequest request, ReadResult *result) {
    uint64_t front = std::min(request.front, mData.count());
    uint64_t back = std::min(request.back, mData.count());

    uint64_t read = back - front;
    memcpy(request.buffer, mData.data() + front, read);

    result->read = read;

    return ERROR_SUCCESS;
}

OsStatus vfs::RamFsFile::write(WriteRequest request, WriteResult *result) {
    uint64_t front = request.front;
    uint64_t back = request.back;

    if (back > mData.count()) {
        mData.resize(back);
    }

    uint64_t write = back - front;
    memcpy(mData.data() + front, request.buffer, write);

    result->written = write;

    return ERROR_SUCCESS;
}

vfs::RamFsFolder::RamFsFolder(RamFsMount *mount)
    : RamFsNode(mount)
{ }

OsStatus vfs::RamFsFolder::create(stdx::StringView, INode **node) {
    if (RamFsNode *child = new(std::nothrow) RamFsFile(mount())) {
        *node = child;
        return ERROR_SUCCESS;
    }

    return ERROR_OUT_OF_MEMORY;
}

OsStatus vfs::RamFsFolder::remove(INode *) {
    return ERROR_NOT_SUPPORTED;
}

OsStatus vfs::RamFsFolder::mkdir(stdx::StringView, INode **node) {
    if (RamFsFolder *child = new(std::nothrow) RamFsFolder(mount())) {
        *node = child;
        return ERROR_SUCCESS;
    }

    return ERROR_OUT_OF_MEMORY;
}

OsStatus vfs::RamFsFolder::rmdir(stdx::StringView) {
    return ERROR_NOT_SUPPORTED;
}

OsStatus vfs::RamFsFolder::find(stdx::StringView, INode **) {
    return ERROR_NOT_FOUND;
}

vfs::RamFsMount::RamFsMount()
    : mRoot(new RamFsFolder(this))
{ }

vfs::IFileSystem *vfs::RamFsMount::filesystem() const {
    return &RamFs::get();
}

sm::SharedPtr<vfs::INode> vfs::RamFsMount::root() const {
    return mRoot;
}

stdx::StringView vfs::RamFs::name() const {
    using namespace stdx::literals;
    return "ramfs"_sv;
}

OsStatus vfs::RamFs::mount(IFileSystemMount **mount) {
    if (RamFsMount *instance = new(std::nothrow) RamFsMount()) {
        *mount = instance;
        return ERROR_SUCCESS;
    }

    return ERROR_OUT_OF_MEMORY;
}

vfs::RamFs& vfs::RamFs::get() {
    return gRamFs;
}
