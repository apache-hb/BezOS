#pragma once

#include "fs2/interface.hpp"
#include "fs2/path.hpp"
#include "fs2/node.hpp"

#include "std/shared_spinlock.hpp"
#include "util/absl.hpp"

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class VfsRoot {
        sm::BTreeMap<VfsPath, std::unique_ptr<IVfsMount>> mMounts;
        std::unique_ptr<IVfsMount> mRootMount;
        std::unique_ptr<INode> mRootNode;

        /// @brief Global lock for the VFS.
        /// @todo Use RCU instead.
        stdx::SharedSpinLock mLock;

        OsStatus walk(const VfsPath& path, INode **parent);

        OsStatus lookupUnlocked(const VfsPath& path, INode **node);

        OsStatus insertMount(INode *parent, const VfsPath& path, std::unique_ptr<IVfsMount> object, IVfsMount **mount);

        OsStatus createFolder(IFolderHandle *folder, VfsString name, INode **node);
        OsStatus createFile(IFolderHandle *folder, VfsString name, INode **node);
        OsStatus addNode(INode *parent, VfsString name, INode *child);

        OsStatus queryFolder(INode *parent, IFolderHandle **handle);
        bool hasInterface(INode *node, sm::uuid uuid, const void *data = nullptr, size_t size = 0);

    public:
        VfsRoot();

        OsStatus addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount);

        template<std::derived_from<IVfsDriver> T>
        OsStatus addMountWithParams(T *driver, const VfsPath& path, IVfsMount **mount, auto&&... args) {
            stdx::UniqueLock guard(mLock);

            //
            // Find the parent to the mount point before creating the mount.
            // This is done as an optimization to avoid creating the mount
            // point if it is not needed.
            //
            INode *parent = nullptr;
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

        OsStatus create(const VfsPath& path, INode **node);
        OsStatus remove(INode *node);

        OsStatus open(const VfsPath& path, IFileHandle **handle);

        OsStatus opendir(const VfsPath& path, IHandle **handle);

        OsStatus mkdir(const VfsPath& path, INode **node);
        OsStatus rmdir(INode *node);

        OsStatus mkpath(const VfsPath& path, INode **node);

        OsStatus lookup(const VfsPath& path, INode **node);

        OsStatus mkdevice(const VfsPath& path, INode *device);
        OsStatus device(const VfsPath& path, sm::uuid interface, const void *data, size_t size, IHandle **handle);
    };
}
