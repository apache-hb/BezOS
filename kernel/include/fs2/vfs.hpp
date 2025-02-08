#pragma once

#include "fs2/path.hpp"

#include "shared/status.h"
#include "std/string.hpp"
#include "std/string_view.hpp"
#include "std/vector.hpp"

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class VfsPath;

    struct VfsHandle;

    struct IVfsNodeIterator;
    struct IVfsNode;

    struct IVfsMount;
    struct IVfsDriver;

    enum class FsNodeType {
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

    struct VfsHandle {
        IVfsNode *node;
    };

    struct IVfsNodeIterator {

    };

    struct IVfsNode {
        /// @brief The name of the entry.
        stdx::String name;

        /// @brief The parent directory of the entry, nullptr if root.
        IVfsNode *parent;

        /// @brief The type of the entry.
        FsNodeType type;

        /// @brief The mount that this node is part of.
        IVfsMount *mount;

        /// @brief Callbacks for the node.
        const VfsNodeCallbacks *callbacks;
    };

    struct IVfsMount {
        const IVfsDriver *driver;

        constexpr IVfsMount(const IVfsDriver *driver)
            : driver(driver)
        { }

        virtual ~IVfsMount() = default;

        virtual OsStatus root([[maybe_unused]] IVfsNode **node) { return OsStatusNotSupported; }
        virtual OsStatus stat([[maybe_unused]] VfsMountStat *stat) { return OsStatusNotSupported; }
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

        virtual OsStatus mount([[maybe_unused]] IVfsMount **mount) { return OsStatusNotSupported; }
        virtual OsStatus unmount([[maybe_unused]] IVfsMount *mount) { return OsStatusNotSupported; }
    };

    class RootVfs {
        stdx::Vector2<std::unique_ptr<IVfsMount>> mMounts;

    public:
        RootVfs();

        OsStatus addMount(IVfsDriver *driver, const VfsPath& path, IVfsMount **mount);

        OsStatus lookup(const VfsPath& path, IVfsNode **node);
    };
}
