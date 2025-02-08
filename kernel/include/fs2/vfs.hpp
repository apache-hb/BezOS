#pragma once

#include "fs2/path.hpp"

#include "shared/status.h"
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

    struct IVfsNodeHandle;

    struct IVfsNodeIterator;
    class IVfsNode;

    struct IVfsMount;
    struct IVfsDriver;

    template<typename TKey, typename TValue, typename TCompare = std::less<TKey>, typename TAllocator = mem::GlobalAllocator<std::pair<const TKey, TValue>>>
    using BTreeMap = absl::btree_map<TKey, TValue, TCompare, TAllocator>;

    struct ReadRequest { };
    struct ReadResult { };

    struct WriteRequest { };
    struct WriteResult { };

    enum class VfsNodeType {
        eNone,
        eFile,
        eFolder,
        eMount,
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

    struct VfsNodeCallbacks {
        OsStatus(*open)();
        OsStatus(*close)();
        OsStatus(*create)();
        OsStatus(*remove)();
        OsStatus(*stat)();
        OsStatus(*read)();
        OsStatus(*write)();

        OsStatus(*mkdir)();
        OsStatus(*rmdir)();
        OsStatus(*lookup)();
        OsStatus(*iterate)();
        OsStatus(*next)();

        OsStatus(*rename)();
    };

    struct IVfsNodeHandle {
        IVfsNode *node;

        OsStatus read(ReadRequest request, ReadResult *result);
        OsStatus write(WriteRequest request, WriteResult *result);
    };

    struct IVfsNodeIterator {
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

        /// @brief The type of the entry.
        VfsNodeType type;

        /// @brief The mount that this node is part of.
        IVfsMount *mount;

        /// @brief Callbacks for the node.
        const VfsNodeCallbacks *callbacks;

        /// @brief If this is a directory, these are all the entries.
        BTreeMap<VfsString, IVfsNode*, std::less<>> children;

        OsStatus open(IVfsNodeHandle **handle);

        OsStatus lookup(VfsStringView name, IVfsNode **child);
        OsStatus addFile(VfsStringView name, IVfsNode **child);
        OsStatus addNode(VfsStringView name, IVfsNode *node);
    };

    struct IVfsMount {
        const IVfsDriver *driver;

        constexpr IVfsMount(const IVfsDriver *driver)
            : driver(driver)
        { }

        virtual ~IVfsMount() = default;

        virtual OsStatus root(IVfsNode**) { return OsStatusNotSupported; }
        virtual OsStatus stat(VfsMountStat*) { return OsStatusNotSupported; }
        virtual OsStatus sync() { return OsStatusNotSupported; }

        virtual OsStatus fid() { return OsStatusNotSupported; }
        virtual OsStatus get() { return OsStatusNotSupported; }
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

    class VfsRoot {
        BTreeMap<VfsPath, std::unique_ptr<IVfsMount>> mMounts;
        IVfsNode mRootNode;

        OsStatus walk(const VfsPath& path, IVfsNode **parent);

    public:
        VfsRoot();

        OsStatus addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount);

        OsStatus createFile(const VfsPath& path, IVfsNode **node);
    };
}
