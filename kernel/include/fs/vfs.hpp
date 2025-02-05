#pragma once

#include "fs/node.hpp"
#include "fs/path.hpp"

namespace km {
    class VirtualFileSystem {
        struct VfsCacheEntry {
            VfsNode *node;
            uint32_t refCount;
        };

        VfsFolder mRoot;
        VfsNodeId mNextId = VfsNodeId(1);

        VfsNodeId allocateId();
        VfsFolder *getParentFolder(const VfsPath& path);

    public:
        VirtualFileSystem() = default;

        VfsNode *mkdir(const VfsPath& path);
        void rmdir(const VfsPath& path);

        VfsNode *open(const VfsPath& path);
        void close(VfsNode *id);

        size_t read(VfsNode *id, void *buffer, size_t size);
        size_t write(VfsNode *id, const void *buffer, size_t size);
    };
}
