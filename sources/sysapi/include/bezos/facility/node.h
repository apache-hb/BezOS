#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsFile Files
/// @{

enum {
    eOsNodeAccessNone = eOsAccessNone,

    eOsNodeAccessRead = eOsAccessRead,
    eOsNodeAccessWrite = eOsAccessWrite,
    eOsNodeAccessStat = eOsAccessStat,
    eOsNodeAccessDestroy = eOsAccessDestroy,
    eOsNodeAccessWait = eOsAccessWait,

    eOsNodeAccessExecute = OS_IMPL_ACCESS_BIT(0),
    eOsNodeAccessQueryInterface = OS_IMPL_ACCESS_BIT(1),

    eOsNodeAccessAll
        = eOsNodeAccessRead
        | eOsNodeAccessWrite
        | eOsNodeAccessStat
        | eOsNodeAccessDestroy
        | eOsNodeAccessWait
        | eOsNodeAccessExecute
        | eOsNodeAccessQueryInterface,
};

typedef OsHandleAccess OsNodeAccess;

/// @brief An invalid file handle.
#define OS_FILE_INVALID ((OsFileHandle)(UINT64_MAX))

/// @brief The path separator used by the operating system.
#define OS_PATH_SEPARATOR '\0'

/// @brief Seek modes.
///
/// Bits [0:2] contain the seek mode.
/// Bits [3:63] are reserved and must be zero.
typedef uint64_t OsSeekMode;

/// @brief Open file modes.
///
/// Bits [0:7] contain the access mode.
/// Bits [8:15] contain the creation mode.
/// Bits [16:63] are reserved and must be zero.
typedef uint64_t OsFileOpenMode;

enum {
    /// @brief Open a file for reading.
    eOsFileRead = (UINT64_C(1) << 0),

    /// @brief Open a file for writing.
    eOsFileWrite = (UINT64_C(1) << 1),

    /// @brief Open a file in append mode.
    /// When specifying this, @a OsFileWrite must also be specified.
    eOsFileAppend = (UINT64_C(1) << 2),

    /// @brief Truncate the file upon opening.
    /// If the file already exists, it is truncated.
    /// When specifying this, @a OsFileWrite must also be specified.
    eOsFileTruncate = (UINT64_C(1) << 3),


    /// @brief Always create a new file.
    /// If the file already exists, it is truncated.
    /// If the file does not exist, then it is created.
    eOsFileCreateAlways = (UINT64_C(1) << 8),

    /// @brief Open an existing file.
    /// If the file does not exist, then the operation fails.
    /// If the file exists, then it is opened.
    eOsFileOpenExisting = (UINT64_C(2) << 8),

    /// @brief Open a file if it exists.
    /// If the file does not exist, then it is created.
    /// If the file exists, then it is opened.
    eOsFileOpenAlways = (UINT64_C(3) << 8),
};

struct OsFileStat {
    /// @brief The logical size of the file.
    ///
    /// This is the size of the files contents ignoring any bookkeeping data.
    uint64_t LogicalSize;

    /// @brief The number of blocks on disk used by the file.
    uint64_t BlockCount;

    /// @brief The size of a block on the disk and filesystem.
    uint64_t BlockSize;

    /// @brief When the file was created.
    OsInstant CreationTime;

    /// @brief The last time the file was read or written.
    OsInstant LastAccessTime;

    /// @brief The last time the file was written to.
    OsInstant LastWriteTime;
};

struct OsFileCreateInfo {
    struct OsPath Path;

    OsFileOpenMode Mode;

    OsProcessHandle Process;
};

struct OsFileReadRequest {
    void *BufferFront;
    void *BufferBack;
    OsInstant Timeout;
};

struct OsFileWriteRequest {
    const void *BufferFront;
    const void *BufferBack;
    OsInstant Timeout;
};

/// @defgroup OsNode VNodes
/// @{

struct OsNodeCreateInfo {
    struct OsPath Path;

    OsProcessHandle Process;

    OsTxHandle Transaction;
};

/// @brief Query for an interface on a vnode.
struct OsNodeQueryInterfaceInfo {
    /// @brief The GUID of the interface to query.
    struct OsGuid InterfaceGuid;

    /// @brief The process to open the interface for.
    OsProcessHandle Process;

    /// @brief The data to open the interface with.
    const void *OpenData;

    /// @brief The size of the open data.
    size_t OpenDataSize;
};

/// @brief Information about a vnode.
struct OsNodeInfo {
    /// @brief The name of the vnode.
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];
};

/// @brief Open a vnode.
///
/// Open a vnode in the filesystem, later this vnode can be queried for interfaces.
///
/// @param CreateInfo The create information for the vnode.
/// @param OutHandle The vnode handle.
///
/// @return The status of the operation.
extern OsStatus OsNodeOpen(struct OsNodeCreateInfo CreateInfo, OsNodeHandle *OutHandle);

/// @brief Close a vnode handle.
///
/// Closes a vnode handle, any device handles will remain valid. The vnode will only
/// be discarded when all device handles are closed.
///
/// @param Handle The vnode handle to close.
///
/// @return The status of the operation.
extern OsStatus OsNodeClose(OsNodeHandle Handle);

/// @brief Query an interface on a vnode.
///
/// Queries an interface on a vnode, this will return a device handle that can be used
/// to interact with the device.
///
/// @param Handle The vnode handle to query the interface on.
/// @param Query The query information for the interface.
/// @param OutHandle The device handle.
///
/// @return The status of the operation.
extern OsStatus OsNodeQueryInterface(OsNodeHandle Handle, struct OsNodeQueryInterfaceInfo Query, OsDeviceHandle *OutHandle);

/// @brief Query information about a vnode.
///
/// Queries information about a vnode, this can be used to get the name of the vnode.
///
/// @param Handle The vnode handle to query.
/// @param OutInfo The information about the vnode.
///
/// @return The status of the operation.
extern OsStatus OsNodeStat(OsNodeHandle Handle, struct OsNodeInfo *OutInfo);

/// @} // group OsNode

#ifdef __cplusplus
}
#endif
