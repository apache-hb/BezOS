#pragma once

#include <bezos/status.h>
#include <bezos/subsystem/identify.h>
#include <bezos/subsystem/fs.h>

#include "fs2/path.hpp"
#include "fs2/base.hpp"

#include "std/string_view.hpp"

namespace km {
    class CallContext;
}

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    struct IVfsMount;
    struct IVfsDriver;

    enum class VfsNodeType {
        eNone,
        eFile,
        eFolder,
        eHardLink,
        eSymbolicLink,
    };

    struct NodeStat {
        uint64_t logical;
        uint64_t blksize;
        uint64_t blocks;
        Access access;
    };

    struct IVfsMount {
        const IVfsDriver *driver;

        constexpr IVfsMount(const IVfsDriver *driver)
            : driver(driver)
        { }

        virtual ~IVfsMount() = default;

        virtual OsStatus mkdir(INode *, VfsStringView, const void *, size_t, INode **) { return OsStatusNotSupported; }
        virtual OsStatus create(INode *, VfsStringView, const void *, size_t, INode **) { return OsStatusNotSupported; }

        virtual OsStatus root(INode **) { return OsStatusNotSupported; }

        virtual OsStatus create(const void *, size_t, INode **) { return OsStatusNotSupported; }
    };

    struct IVfsDriver {
        stdx::StringView name;

        constexpr IVfsDriver(stdx::StringView name)
            : name(name)
        { }

        virtual ~IVfsDriver() = default;

        virtual OsStatus mount(IVfsMount **) { return OsStatusNotSupported; }
        virtual OsStatus unmount(IVfsMount *) { return OsStatusNotSupported; }
    };

    template<typename T> requires (std::is_trivial_v<T>)
    OsStatus ReadObject(IHandle *handle, T *object, uint64_t offset) {
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
    OsStatus ReadArray(IHandle *handle, T *array, size_t count, uint64_t offset) {
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
}
