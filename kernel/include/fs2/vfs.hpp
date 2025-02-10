#pragma once

#include "fs2/path.hpp"
#include "fs2/node.hpp"

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class VfsRoot {
        BTreeMap<VfsPath, std::unique_ptr<IVfsMount>> mMounts;
        std::unique_ptr<IVfsMount> mRootMount;
        std::unique_ptr<IVfsNode> mRootNode;

        OsStatus walk(const VfsPath& path, IVfsNode **parent);

    public:
        VfsRoot();

        OsStatus addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount);

        OsStatus create(const VfsPath& path, IVfsNode **node);
        OsStatus remove(IVfsNode *node);

        OsStatus open(const VfsPath& path, IVfsNodeHandle **handle);

        OsStatus mkdir(const VfsPath& path, IVfsNode **node);
        OsStatus rmdir(IVfsNode *node);

        OsStatus mkpath(const VfsPath& path, IVfsNode **node);

        OsStatus lookup(const VfsPath& path, IVfsNode **node);
    };
}
