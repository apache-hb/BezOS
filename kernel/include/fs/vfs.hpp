#pragma once

#include "fs/node.hpp"
#include "fs/path.hpp"

namespace km {
    class VfsHandle {
        vfs::INode *mNode;

        uint64_t mOffset;

    public:
        VfsHandle(vfs::INode *node)
            : mNode(node)
        { }

        KmStatus read(void *buffer, size_t size, size_t *result);
        KmStatus write(const void *buffer, size_t size, size_t *result);
    };

    class VirtualFileSystem {
        struct VfsCacheEntry {
            VfsEntry *entry;
            uint32_t refCount;
        };

        std::unique_ptr<vfs::IFileSystemMount> mRootMount;
        VfsEntry mRootEntry;

        std::tuple<VfsEntry*, vfs::IFileSystemMount*> getParentFolder(const VfsPath& path);

    public:
        VirtualFileSystem();

        VfsEntry *mkdir(const VfsPath& path);
        void rmdir(const VfsPath& path);

        VfsEntry *mount(const VfsPath& path, vfs::IFileSystem *fs);
        void unmount(const VfsPath& path);

        VfsEntry *open(const VfsPath& path);
        void close(VfsEntry *id);

        size_t read(VfsEntry *id, void *buffer, size_t size);
        size_t write(VfsEntry *id, const void *buffer, size_t size);
    };
}
