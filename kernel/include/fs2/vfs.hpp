#pragma once

#include "fs2/path.hpp"
#include "fs2/node.hpp"
#include "std/spinlock.hpp"

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class VfsRoot {
        sm::BTreeMap<VfsPath, std::unique_ptr<IVfsMount>> mMounts;
        std::unique_ptr<IVfsMount> mRootMount;
        std::unique_ptr<IVfsNode> mRootNode;

        /// @brief Global lock for the VFS.
        /// @todo Use RCU instead.
        stdx::SpinLock mLock;

        OsStatus walk(const VfsPath& path, IVfsNode **parent);

        OsStatus lookupUnlocked(const VfsPath& path, IVfsNode **node);

        OsStatus insertMount(IVfsNode *parent, const VfsPath& path, std::unique_ptr<IVfsMount> object, IVfsMount **mount);

    public:
        VfsRoot();

        OsStatus addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount);

        template<std::derived_from<IVfsDriver> T>
        OsStatus addMountWithParams(T *driver, const VfsPath& path, IVfsMount **mount, auto&&... args) {
            stdx::LockGuard guard(mLock);

            //
            // Find the parent to the mount point before creating the mount.
            // This is done as an optimization to avoid creating the mount
            // point if it is not needed.
            //
            IVfsNode *parent = nullptr;
            if (OsStatus status = walk(path, &parent)) {
                return status;
            }

            //
            // Create the mount point and pass along the provided arguments.
            //
            std::unique_ptr<IVfsMount> impl;
            if (OsStatus status = driver->createMount(std::out_ptr(impl), std::forward<decltype(args)>(args)...)) {
                return status;
            }

            //
            // Hand off the created mount point to complete the mount operation.
            //
            return insertMount(parent, path, std::move(impl), mount);
        }

        OsStatus create(const VfsPath& path, IVfsNode **node);
        OsStatus remove(IVfsNode *node);

        OsStatus open(const VfsPath& path, IVfsNodeHandle **handle);

        OsStatus mkdir(const VfsPath& path, IVfsNode **node);
        OsStatus rmdir(IVfsNode *node);

        OsStatus mkpath(const VfsPath& path, IVfsNode **node);

        OsStatus lookup(const VfsPath& path, IVfsNode **node);
    };
}
