#pragma once

#include <bezos/status.h>
#include <bezos/subsystem/identify.h>
#include <bezos/subsystem/fs.h>

#include "fs2/path.hpp"
#include "util/util.hpp"
#include "util/uuid.hpp"

namespace km {
    class SystemObjects;
}

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class INode;
    class IHandle;

    struct IVfsMount;

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
        OsGuid guid;
    };

    struct NodeInfo {
        stdx::StringView name;
        IVfsMount *mount;
        INode *parent;
        Access access;
        uint32_t generation;
    };

    class IInvokeContext {
    public:
        virtual ~IInvokeContext() = default;

        /// @brief The process that is invoking the method.
        virtual OsProcessHandle process() = 0;

        /// @brief The thread that is invoking the method.
        virtual OsThreadHandle thread() = 0;

        /// @brief Insert or retrieve the handle associated with this node.
        ///
        /// @param node The node to resolve.
        ///
        /// @return The handle associated with the node.
        virtual OsNodeHandle resolveNode(INode *node) = 0;
    };

    /// @brief A handle to a file or folder.
    ///
    /// @note All nodes must provide @a kOsIdentifyGuid.
    class IHandle {
    public:
        virtual ~IHandle() = default;

        /// @brief Invoke a method on the handle.
        ///
        /// @param context the invocation context.
        /// @param method The method to invoke.
        /// @param data The data to pass to the method.
        /// @param size The size of the data.
        ///
        /// @return The status of the invocation.
        virtual OsStatus invoke(IInvokeContext *, uint64_t, void *, size_t) { return OsStatusFunctionNotSupported; }

        /// @brief Read data from the handle.
        ///
        /// @param request The read request.
        /// @param result The read result.
        ///
        /// @return The status of the read operation.
        virtual OsStatus read(ReadRequest, ReadResult *) { return OsStatusNotSupported; }

        /// @brief Write data to the handle.
        ///
        /// @param request The write request.
        /// @param result The write result.
        ///
        /// @return The status of the write operation.
        virtual OsStatus write(WriteRequest, WriteResult *) { return OsStatusNotSupported; }

        /// @brief Get information about the handle.
        ///
        /// @return The information about the handle.
        virtual HandleInfo info() = 0;
    };

    class INode {
    public:
        virtual ~INode() = default;

        /// @brief Query this node for an interface.
        ///
        /// If this function succeeds, the handle must be released by the caller.
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
        virtual OsStatus query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) = 0;

        /// @brief Connect a node to its parent.
        ///
        /// @param parent The parent node.
        /// @param name The name of the node.
        /// @param access The access rights of the node.
        virtual void init(INode *parent, VfsString name, Access access) = 0;

        /// @brief Get information about the node.
        ///
        /// @return The information about the node.
        virtual NodeInfo info() = 0;

        /// @brief Retain a reference to the handle.
        ///
        /// Increment the public reference count of the handle.
        virtual void retain() = 0;

        /// @brief Release a reference to the handle.
        ///
        /// If this function returns 0 the handle is put into a destroy state
        /// and must not be used, all handles that reference this node will
        /// remain valid. When the last handle referencing the node is released
        /// the node will be destroyed.
        ///
        /// @return The number of references remaining.
        virtual unsigned release() = 0;
    };
}
