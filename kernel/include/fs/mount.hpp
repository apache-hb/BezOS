#pragma once

#include "shared/status.h"

#include "std/shared.hpp"
#include "std/string_view.hpp"

#include "util/cxx_chrono.hpp" // IWYU pragma: keep

#include "fs/types.hpp"

namespace vfs {
    class INode;
    class IFileSystemMount;
    class IFileSystem;

    struct ReadRequest {
        uint64_t front;
        uint64_t back;
        std::byte *buffer;
    };

    struct ReadResult {
        uint64_t read;
    };

    struct WriteRequest {
        uint64_t front;
        uint64_t back;
        const std::byte *buffer;
    };

    struct WriteResult {
        uint64_t written;
    };

    struct OpenRequest {

    };

    struct OpenResult {

    };

    struct CloseRequest {

    };

    struct CloseResult {

    };

    using FileTime = std::chrono::utc_clock::time_point;

    struct NodeAttributes {
        FileTime created;
        FileTime modified;
        FileTime accessed;
        uint64_t filesize;
        uint64_t blocks;
        km::VfsNodeId id;
        km::VfsEntryType type;
    };

    class INodeIterator {
    public:
        virtual ~INodeIterator() = default;

        virtual OsStatus next(INode **node) = 0;
    };

    /// @brief A file or directory in a mounted filesystem.
    class INode {
    public:
        virtual ~INode() = default;

        virtual IFileSystemMount *owner() const = 0;
        virtual km::VfsEntryType type() const = 0;

        virtual OsStatus stat(NodeAttributes*) { return OsStatusNotSupported; }

        virtual OsStatus read(ReadRequest request, ReadResult *result) = 0;
        virtual OsStatus write(WriteRequest request, WriteResult *result) = 0;

        virtual OsStatus create(stdx::StringView name, INode **node) = 0;
        virtual OsStatus remove(INode *node) = 0;

        virtual OsStatus mkdir(stdx::StringView name, INode **node) = 0;
        virtual OsStatus rmdir(stdx::StringView name) = 0;

        virtual OsStatus find(stdx::StringView name, INode **node) = 0;
    };

    class EmptyNode : public INode {
        OsStatus read(ReadRequest, ReadResult *) override {
            return OsStatusNotSupported;
        }

        OsStatus write(WriteRequest, WriteResult *) override {
            return OsStatusNotSupported;
        }

        OsStatus create(stdx::StringView, INode **) override {
            return OsStatusNotSupported;
        }

        OsStatus remove(INode *) override {
            return OsStatusNotSupported;
        }

        OsStatus mkdir(stdx::StringView, INode **) override {
            return OsStatusNotSupported;
        }

        OsStatus rmdir(stdx::StringView) override {
            return OsStatusNotSupported;
        }

        OsStatus find(stdx::StringView, INode **) override {
            return OsStatusNotSupported;
        }
    };

    /// @brief An instance of a filesystem thats been mounted in the VFS.
    class IFileSystemMount {
    public:
        virtual ~IFileSystemMount() = default;

        virtual IFileSystem *filesystem() const = 0;

        virtual sm::SharedPtr<INode> root() const = 0;
    };

    /// @brief Represents a file system type.
    class IFileSystem {
    public:
        virtual ~IFileSystem() = default;

        virtual stdx::StringView name() const = 0;

        virtual OsStatus mount(IFileSystemMount **mount) = 0;
    };
}
