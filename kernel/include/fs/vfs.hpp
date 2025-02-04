#pragma once

#include "fs/node.hpp"
#include "fs/path.hpp"

namespace km {
    class VirtualFileSystem {
        struct VfsCacheEntry {
            VfsNode *node;
            uint32_t refCount;
        };

        BTreeMap<VfsNodeId, VfsCacheEntry> mNodeCache;

        VfsFolder mRoot;
        VfsNodeId mNextId = VfsNodeId(1);

        VfsNodeId allocateId();
        VfsFolder *getParentFolder(const VfsPath& path);
        VfsFile *findFileInCache(VfsNodeId id);
        void cacheNode(VfsNodeId id, VfsNode *node);

    public:
        VirtualFileSystem() = default;

        VfsNodeId mkdir(const VfsPath& path);

        void rmdir(const VfsPath& path);

        VfsNodeId open(const VfsPath& path);

        void close(VfsNodeId id);

        size_t read(VfsNodeId id, void *buffer, size_t size);
        size_t write(VfsNodeId id, const void *buffer, size_t size);
    };
}
