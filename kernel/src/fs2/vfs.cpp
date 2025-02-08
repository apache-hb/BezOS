#include "fs2/vfs.hpp"

using namespace vfs2;

RootVfs::RootVfs() { }

OsStatus RootVfs::addMount(IVfsDriver *driver, const VfsPath&, IVfsMount **mount) {
    std::unique_ptr<IVfsMount> impl;
    if (OsStatus status = driver->mount(std::out_ptr(impl))) {
        return status;
    }

    auto ref = mMounts.emplace_back(std::move(impl));
    *mount = ref->get();

    return OsStatusSuccess;
}
