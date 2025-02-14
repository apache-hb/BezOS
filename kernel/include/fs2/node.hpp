#pragma once

#include <atomic>
#include <bezos/status.h>

#include "fs2/path.hpp"

#include "std/spinlock.hpp"
#include "std/string_view.hpp"

#include "absl/container/btree_map.h"
#include "util/util.hpp"

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class VfsPath;

    class IVfsNodeHandle;

    class IVfsNodeIterator;
    class IVfsNode;

    struct IVfsMount;
    struct IVfsDriver;

    template<typename TKey, typename TValue, typename TCompare = std::less<TKey>, typename TAllocator = mem::GlobalAllocator<std::pair<const TKey, TValue>>>
    using BTreeMap = absl::btree_map<TKey, TValue, TCompare, TAllocator>;

    enum class VfsNodeId : uint64_t { };

    struct ReadRequest {
        void *begin;
        void *end;
        uint64_t offset;
    };

    struct ReadResult {
        uint64_t read;
    };

    struct WriteRequest {
        const void *begin;
        const void *end;
        uint64_t offset;
    };

    struct WriteResult {
        uint64_t write;
    };

    enum class VfsNodeType {
        eNone,
        eFile,
        eFolder,
    };

    struct VfsMountStat {
        /// @brief The maximum size of the filesystem.
        uint64_t maxMountSize;

        /// @brief The maximum size of a single file.
        uint64_t maxFileSize;

        /// @brief The maximum length of a path segment.
        uint16_t maxNameSize;

        /// @brief The size of a block in the mount.
        uint16_t blockSize;
    };

    struct VfsNodeStat {
        uint64_t size;
    };

    class IVfsNodeHandle {
    public:
        virtual ~IVfsNodeHandle() = default;

        IVfsNodeHandle(IVfsNode *it)
            : node(it)
            , offset(0)
        { }

        IVfsNode *node;
        uint64_t offset;

        virtual OsStatus read(ReadRequest request, ReadResult *result);
        virtual OsStatus write(WriteRequest request, WriteResult *result);
        virtual OsStatus stat(VfsNodeStat *stat);
    };

    template<typename T> requires (std::is_trivial_v<T>)
    OsStatus ReadObject(IVfsNodeHandle *handle, T *object, uint64_t offset) {
        ReadRequest request {
            .begin = object,
            .end = (std::byte*)object + sizeof(T),
            .offset = offset,
        };

        ReadResult result{};
        if (OsStatus status = handle->read(request, &result)) {
            return status;
        }

        if (result.read != sizeof(T)) {
            return OsStatusEndOfFile;
        }

        return OsStatusSuccess;
    }

    template<typename T> requires (std::is_trivial_v<T>)
    OsStatus ReadArray(IVfsNodeHandle *handle, T *array, size_t count, uint64_t offset) {
        ReadRequest request {
            .begin = array,
            .end = (std::byte*)array + sizeof(T) * count,
            .offset = offset,
        };

        ReadResult result{};
        if (OsStatus status = handle->read(request, &result)) {
            return status;
        }

        if (result.read != sizeof(T) * count) {
            return OsStatusEndOfFile;
        }

        return OsStatusSuccess;
    }

    class IVfsNodeIterator {
    public:
        virtual ~IVfsNodeIterator() = default;
    };

    class IVfsNode {
    public:
        IVfsNode() = default;

        virtual ~IVfsNode();

        UTIL_NOCOPY(IVfsNode);
        UTIL_NOMOVE(IVfsNode);

        /// @brief The name of the entry.
        VfsString name;

        /// @brief The parent directory of the entry, nullptr if root.
        IVfsNode *parent;

        /// @brief The mount that this node is part of.
        IVfsMount *mount;

        /// @brief If this is a directory, these are all the entries.
        BTreeMap<VfsString, IVfsNode*, std::less<>> children;

        /// @brief The type of the entry.
        VfsNodeType type;

        virtual OsStatus read(ReadRequest, ReadResult*) { return OsStatusNotSupported; }
        virtual OsStatus write(WriteRequest, WriteResult*) { return OsStatusNotSupported; }

        virtual OsStatus create(IVfsNode**) { return OsStatusNotSupported; }
        virtual OsStatus mkdir(IVfsNode**) { return OsStatusNotSupported; }

        virtual OsStatus remove(IVfsNode*) { return OsStatusNotSupported; }
        virtual OsStatus rmdir(IVfsNode*) { return OsStatusNotSupported; }

        virtual OsStatus stat(VfsNodeStat*) { return OsStatusNotSupported; }

        OsStatus open(IVfsNodeHandle **handle);

        OsStatus lookup(VfsStringView name, IVfsNode **child);
        OsStatus addFile(VfsStringView name, IVfsNode **child);
        OsStatus addFolder(VfsStringView name, IVfsNode **child);
        OsStatus addNode(VfsStringView name, IVfsNode *node);

        void initNode(IVfsNode *node, VfsStringView name, VfsNodeType type);

        bool readyForRemove() const { return true; }
    };

    struct IVfsMount {
        const IVfsDriver *driver;

        constexpr IVfsMount(const IVfsDriver *driver)
            : driver(driver)
        { }

        virtual ~IVfsMount() = default;

        virtual OsStatus root(IVfsNode**) { return OsStatusNotSupported; }
    };

    struct IVfsDriver {
        stdx::StringView name;

        constexpr IVfsDriver(stdx::StringView name)
            : name(name)
        { }

        virtual ~IVfsDriver() = default;

        virtual OsStatus mount(IVfsMount**) { return OsStatusNotSupported; }
        virtual OsStatus unmount(IVfsMount*) { return OsStatusNotSupported; }
    };
}
