#pragma once

#include <bezos/status.h>
#include <bezos/subsystem/identify.h>
#include <bezos/subsystem/fs.h>

#include "fs2/path.hpp"
#include "util/util.hpp"
#include "util/uuid.hpp"

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    struct IVfsMount;
    class INode;

    struct ReadRequest {
        void *begin;
        void *end;
        uint64_t offset;
        OsInstant timeout;

        uintptr_t size() const { return (std::byte*)end - (std::byte*)begin; }
    };

    struct ReadResult {
        uint64_t read;
    };

    struct WriteRequest {
        const void *begin;
        const void *end;
        uint64_t offset;
        OsInstant timeout;

        uintptr_t size() const { return (std::byte*)end - (std::byte*)begin; }
    };

    enum class Access {
        eNone = 0,

        eRead = (1 << 0),
        eWrite = (1 << 1),
        eExecute = (1 << 2),

        R = eRead,
        W = eWrite,
        X = eExecute,
        RW = eRead | eWrite,
        RX = eRead | eExecute,
        RWX = eRead | eWrite | eExecute,
    };

    UTIL_BITFLAGS(Access);

    struct WriteResult {
        uint64_t write;
    };

    struct HandleInfo {
        INode *node;
    };

    struct NodeInfo {
        stdx::StringView name;
        IVfsMount *mount;
        INode *parent;
        Access access;
    };

    /// @brief A handle to a file or folder.
    ///
    /// @note All handles that advertise as @a kOsFolderGuid must inherit from @a IFolderHandle.
    /// @note All handles that advertise as @a kOsFileGuid must inherit from @a IFileHandle.
    class IHandle {
    public:
        virtual ~IHandle() = default;

        /// @brief Invoke a method on the handle.
        ///
        /// @param method The method to invoke.
        /// @param data The data to pass to the method.
        /// @param size The size of the data.
        ///
        /// @return The status of the invocation.
        virtual OsStatus invoke(uint64_t, void *, size_t) { return OsStatusNotSupported; }

        virtual OsStatus read(ReadRequest, ReadResult *) { return OsStatusNotSupported; }
        virtual OsStatus write(WriteRequest, WriteResult *) { return OsStatusNotSupported; }

        virtual HandleInfo info() = 0;
    };

    class INode {
        public:
        virtual ~INode() = default;

        /// @brief Query this node for an interface.
        ///
        /// @param uuid The interface to query for.
        /// @param data The data to pass to the interface.
        /// @param size The size of the data.
        /// @param handle The handle to the interface.
        ///
        /// @retval OsStatusSuccess The interface was found, and the handle has been set.
        /// @retval OsStatusInterfaceNotSupported The interface is not supported.
        ///
        /// @return The status of the query operation.
        virtual OsStatus query(sm::uuid, const void *, size_t, IHandle **) = 0;

        virtual void init(INode *parent, VfsString name, Access access) = 0;

        virtual NodeInfo info() = 0;
    };
}
