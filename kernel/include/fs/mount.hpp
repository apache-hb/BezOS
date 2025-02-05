#pragma once

#include "shared/status.h"

#include "std/string_view.hpp"

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

    /// @brief A file or directory in a mounted filesystem.
    class INode {
    public:
        virtual ~INode() = default;

        virtual IFileSystemMount *owner() const = 0;

        virtual KmStatus read(ReadRequest request, ReadResult *result) = 0;
        virtual KmStatus write(WriteRequest request, WriteResult *result) = 0;

        virtual KmStatus create(stdx::StringView name, INode **node) = 0;
        virtual KmStatus remove(INode *node) = 0;

        virtual KmStatus mkdir(stdx::StringView name, INode **node) = 0;
        virtual KmStatus rmdir(stdx::StringView name) = 0;

        virtual KmStatus find(stdx::StringView name, INode **node) = 0;
    };

    class EmptyNode : public INode {
        KmStatus read(ReadRequest, ReadResult *) override {
            return ERROR_NOT_SUPPORTED;
        }

        KmStatus write(WriteRequest, WriteResult *) override {
            return ERROR_NOT_SUPPORTED;
        }

        KmStatus create(stdx::StringView, INode **) override {
            return ERROR_NOT_SUPPORTED;
        }

        KmStatus remove(INode *) override {
            return ERROR_NOT_SUPPORTED;
        }

        KmStatus mkdir(stdx::StringView, INode **) override {
            return ERROR_NOT_SUPPORTED;
        }

        KmStatus rmdir(stdx::StringView) override {
            return ERROR_NOT_SUPPORTED;
        }

        KmStatus find(stdx::StringView, INode **) override {
            return ERROR_NOT_SUPPORTED;
        }
    };

    /// @brief An instance of a filesystem thats been mounted in the VFS.
    class IFileSystemMount {
    public:
        virtual ~IFileSystemMount() = default;

        virtual IFileSystem *filesystem() const = 0;

        virtual INode *root() const = 0;
    };

    /// @brief Represents a file system type.
    class IFileSystem {
    public:
        virtual ~IFileSystem() = default;

        virtual stdx::StringView name() const = 0;

        virtual KmStatus mount(IFileSystemMount **mount) = 0;
    };
}
