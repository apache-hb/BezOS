#pragma once

#include "fs2/interface.hpp"
#include "fs2/path.hpp"
#include "fs2/node.hpp"

#include "std/shared_spinlock.hpp"
#include "util/absl.hpp"

#include <memory>

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class VfsRoot {
        sm::RcuDomain mDomain;

        sm::BTreeMap<VfsPath, std::unique_ptr<IVfsMount>> mMounts GUARDED_BY(mLock);
        std::unique_ptr<IVfsMount> mRootMount;
        sm::RcuSharedPtr<INode> mRootNode;

        /// @brief Global lock for the VFS.
        stdx::SharedSpinLock mLock;

        OsStatus walk(const VfsPath& path, sm::RcuSharedPtr<INode> *parent);

        OsStatus lookupUnlocked(const VfsPath& path, sm::RcuSharedPtr<INode> *node);

        OsStatus insertMount(sm::RcuSharedPtr<INode> parent, const VfsPath& path, std::unique_ptr<IVfsMount> object, IVfsMount **mount);

        OsStatus createFolder(IFolderHandle *folder, VfsString name, sm::RcuSharedPtr<INode> *node);
        OsStatus createFile(IFolderHandle *folder, VfsString name, sm::RcuSharedPtr<INode> *node);
        OsStatus addNode(sm::RcuSharedPtr<INode> parent, VfsString name, sm::RcuSharedPtr<INode> child);

        OsStatus queryFolder(sm::RcuSharedPtr<INode> parent, IFolderHandle **handle);
        bool hasInterface(sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data = nullptr, size_t size = 0);

    public:
        VfsRoot();

        OsStatus addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount);

        template<std::derived_from<IVfsDriver> T>
        OsStatus addMountWithParams(T *driver, const VfsPath& path, IVfsMount **mount, auto&&... args) {
            //
            // Find the parent to the mount point before creating the mount.
            // This is done as an optimization to avoid creating the mount
            // point if it is not needed.
            //
            sm::RcuSharedPtr<INode> parent = nullptr;
            if (OsStatus status = walk(path, &parent)) {
                return status;
            }

            //
            // Create the mount point and pass along the provided arguments.
            //
            std::unique_ptr<IVfsMount> impl;
            if (OsStatus status = driver->createMount(std::out_ptr(impl), &mDomain, std::forward<decltype(args)>(args)...)) {
                return status;
            }

            //
            // Hand off the created mount point to complete the mount operation.
            //
            return insertMount(parent, path, std::move(impl), mount);
        }

        OsStatus create(const VfsPath& path, sm::RcuSharedPtr<INode> *node);
        OsStatus remove(sm::RcuSharedPtr<INode> node);

        OsStatus open(const VfsPath& path, IFileHandle **handle);

        OsStatus opendir(const VfsPath& path, IHandle **handle);

        OsStatus mkdir(const VfsPath& path, sm::RcuSharedPtr<INode> *node);
        OsStatus rmdir(sm::RcuSharedPtr<INode> node);

        OsStatus mkpath(const VfsPath& path, sm::RcuSharedPtr<INode> *node);

        OsStatus lookup(const VfsPath& path, sm::RcuSharedPtr<INode> *node);

        OsStatus mkdevice(const VfsPath& path, sm::RcuSharedPtr<INode> device);
        OsStatus device(const VfsPath& path, sm::uuid interface, const void *data, size_t size, IHandle **handle);

        void synchronize() {
            mDomain.synchronize();
        }

        sm::RcuDomain *domain() {
            return &mDomain;
        }
    };
}
