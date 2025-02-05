#pragma once

#include "fs/node.hpp"
#include "fs/path.hpp"

namespace km {
    class VfsHandle {
        VfsEntry *mNode;

        uint64_t mOffset;

    public:
        VfsHandle(VfsEntry *node)
            : mNode(node)
            , mOffset(0)
        { }

        KmStatus read(void *buffer, size_t size, size_t *result) {
            vfs::ReadRequest request = {
                .front = mOffset,
                .back = mOffset + size,
                .buffer = (std::byte*)buffer,
            };

            vfs::ReadResult readResult;

            KmStatus status = mNode->node()->read(request, &readResult);
            if (status == ERROR_SUCCESS) {
                mOffset += readResult.read;
                *result = readResult.read;
            }

            return status;
        }

        KmStatus write(const void *buffer, size_t size, size_t *result) {
            vfs::WriteRequest request = {
                .front = mOffset,
                .back = mOffset + size,
                .buffer = (const std::byte*)buffer,
            };

            vfs::WriteResult writeResult;

            KmStatus status = mNode->node()->write(request, &writeResult);
            if (status == ERROR_SUCCESS) {
                mOffset += writeResult.written;
                *result = writeResult.written;
            }

            return status;
        }
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

        VfsHandle *open(const VfsPath& path);
        void close(VfsHandle *id);
    };
}
