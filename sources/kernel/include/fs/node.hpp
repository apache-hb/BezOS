#pragma once

#include <bezos/status.h>
#include <bezos/subsystem/identify.h>
#include <bezos/subsystem/fs.h>

#include "fs/path.hpp"
#include "fs/base.hpp"

#include "std/string_view.hpp"

namespace km {
    class CallContext;
}

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs {
    class IVfsMount;
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
        sys::NodeAccess access;
    };

    class IVfsMount {
    protected:
        const IVfsDriver *mDriver;
        sm::RcuDomain *mDomain;

        constexpr IVfsMount(const IVfsDriver *driver, sm::RcuDomain *domain)
            : mDriver(driver)
            , mDomain(domain)
        { }

    public:
        virtual ~IVfsMount() = default;

        virtual OsStatus mkdir(sm::RcuSharedPtr<INode>, VfsStringView, const void *, size_t, sm::RcuSharedPtr<INode> *) { return OsStatusNotSupported; }
        virtual OsStatus create(sm::RcuSharedPtr<INode>, VfsStringView, const void *, size_t, sm::RcuSharedPtr<INode> *) { return OsStatusNotSupported; }

        virtual OsStatus root(sm::RcuSharedPtr<INode> *) { return OsStatusNotSupported; }

        virtual OsStatus create(const void *, size_t, sm::RcuSharedPtr<INode> *) { return OsStatusNotSupported; }

        sm::RcuDomain *domain() const { return mDomain; }
    };

    struct IVfsDriver {
        stdx::StringView name;

        constexpr IVfsDriver(stdx::StringView name)
            : name(name)
        { }

        virtual ~IVfsDriver() = default;

        virtual OsStatus mount(sm::RcuDomain *, IVfsMount **) { return OsStatusNotSupported; }
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
