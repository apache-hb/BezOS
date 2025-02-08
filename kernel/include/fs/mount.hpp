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
        uint64_t write;
    };

    struct OpenRequest {

    };

    struct OpenResult {

    };

    struct CloseRequest {

    };

    struct CloseResult {

    };

    struct CreateFolderRequest {

    };

    struct CreateFolderResult {

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

        virtual OsStatus read(ReadRequest, ReadResult*) { return OsStatusNotSupported; }
        virtual OsStatus write(WriteRequest, WriteResult*) { return OsStatusNotSupported; }

        virtual OsStatus open(OpenRequest, OpenResult*) { return OsStatusNotSupported; }
        virtual OsStatus close(CloseRequest, CloseResult*) { return OsStatusNotSupported; }

        virtual OsStatus create(stdx::StringView, INode **) { return OsStatusNotSupported; }
        virtual OsStatus remove(INode *) { return OsStatusNotSupported; }

        virtual OsStatus mkdir(CreateFolderRequest, CreateFolderResult*) { return OsStatusNotSupported; }

        virtual OsStatus mkdir(stdx::StringView, INode **) { return OsStatusNotSupported; }

        virtual OsStatus find(stdx::StringView, INode **) { return OsStatusNotSupported; }

        virtual OsStatus iterate(INodeIterator**) { return OsStatusNotSupported; }
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
