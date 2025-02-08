#pragma once

#include "shared/status.h"
#include "std/string.hpp"
#include "std/string_view.hpp"

namespace vfs2 {
    struct FsHandle;
    struct FsNode;
    struct FsMount;
    struct FsDriver;

    class VfsPath;
    struct VfsEntry;
    struct Vfs;

    enum class FsNodeType {
        eNone,
        eFile,
        eFolder,
        eMount,
    };

    struct FsStat {
        __uint128_t maxMountSize;
        __uint128_t maxFileSize;
        uint16_t maxNameSize;
    };

    struct FsNodeCallbacks {
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

    struct FsHandleCallbacks {

    };

    struct FsHandle {
        FsNode *node;
    };

    struct FsNode {
        FsNodeType type;
        FsMount *mount;
        const FsNodeCallbacks *callbacks;
    };

    struct FsMount {
        const FsDriver *driver;
    };

    struct FsDriver {
        stdx::StringView name;

        OsStatus(*mount)();
        OsStatus(*unmount)();
        OsStatus(*root)();
        OsStatus(*stat)(const FsMount *mount, FsStat *stat);
        OsStatus(*sync)();
        OsStatus(*fid)();
        OsStatus(*get)();
    };





    class VfsPath {

    };

    struct VfsEntry {
        stdx::String name;
        VfsEntry *parent;
        FsNode *node;
    };

    struct Vfs {

    };
}
