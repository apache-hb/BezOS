#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsFile Files
/// @{

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
    /// This implies @a OsFileWrite.
    eOsFileAppend = (UINT64_C(1) << 2),

    /// @brief Truncate the file upon opening.
    /// This implies @a OsFileWrite.
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

enum {
    /// @brief Seek relative to the beginning of the file.
    eOsSeekAbsolute = UINT64_C(0),

    /// @brief Seek relative to the current position.
    eOsSeekRelative = UINT64_C(1),

    /// @brief Seek relative to the end of the file.
    eOsSeekEnd      = UINT64_C(2),
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

struct OsFileSeekRequest {
    OsSeekMode Mode;
    int64_t Offset;
};

/// @brief Open or create a file.
///
/// @pre @p CreateInfo.PathFront and @p CreateInfo.PathBack must point to the front and back of a valid buffer.
/// @pre @p CreateInfo.Mode must be a valid combination of @a OsFileOpenMode flags.
///
/// @post @p OutHandle will be a valid file handle if the operation is successful.
/// @post @p OutHandle will be @a OsFileHandleInvalid if the operation fails.
///
/// @param CreateInfo The create information for the file.
/// @param OutHandle The file handle.
///
/// @return The status of the operation.
extern OsStatus OsFileOpen(struct OsFileCreateInfo CreateInfo, OsFileHandle *OutHandle);

/// @brief Close a file handle.
///
/// @pre @p Handle must be a valid file handle.
///
/// @param Handle The file handle to close.
///
/// @return The status of the operation.
extern OsStatus OsFileClose(OsFileHandle Handle);

/// @brief Read from a file.
///
/// @pre @p Handle must be a valid file handle.
/// @pre @p BufferFront and @p BufferBack must point to the front and back of a valid buffer.
/// @pre @p BufferBack must be > @p BufferFront.
/// @pre @p OutRead must point to valid memory.
///
/// @post @p OutRead will contain the number of bytes read.
/// @post @p BufferFront to *@p OutRead will contain the data read, the remaining data is undefined.
///
/// @param Handle The file handle to read from.
/// @param BufferFront The front of the buffer to read into.
/// @param BufferBack The back of the buffer to read into.
/// @param OutRead The number of bytes read.
///
/// @return The status of the operation.
extern OsStatus OsFileRead(OsFileHandle Handle, void *BufferFront, void *BufferBack, uint64_t *OutRead);

/// @brief Write to a file.
///
/// @pre @p Handle must be a valid file handle.
/// @pre @p BufferFront and @p BufferBack must point to the front and back of a valid buffer.
/// @pre @p BufferBack must be > @p BufferFront.
/// @pre @p OutWritten must point to valid memory.
///
/// @post @p OutWritten will contain the number of bytes written.
///
/// @param Handle The file handle to write to.
/// @param BufferFront The front of the buffer to write from.
/// @param BufferBack The back of the buffer to write from.
/// @param OutWritten The number of bytes written.
///
/// @return The status of the operation.
extern OsStatus OsFileWrite(OsFileHandle Handle, const void *BufferFront, const void *BufferBack, uint64_t *OutWritten);

/// @brief Seek to a position in a file.
///
/// @pre @p Handle must be a valid file handle.
/// @pre @p Mode must be a valid seek mode.
/// @pre @p OutPosition must point to valid memory.
///
/// @post @p OutPosition will contain the new position in the file.
///
/// @param Handle The file handle to seek in.
/// @param Mode The seek mode.
/// @param Offset The offset to seek to.
/// @param OutPosition The new position in the file.
///
/// @return The status of the operation.
extern OsStatus OsFileSeek(OsFileHandle Handle, OsSeekMode Mode, int64_t Offset, uint64_t *OutPosition);

extern OsStatus OsFileStat(OsFileHandle Handle, struct OsFileStat *OutStat);

/// @} // group OsFile


/// @defgroup OsNode VNodes
/// @{

struct OsNodeCreateInfo {
    struct OsPath Path;
    OsProcessHandle Process;
};

struct OsNodeQueryInterfaceInfo {
    struct OsGuid InterfaceGuid;
    OsProcessHandle Process;
    const void *OpenData;
    size_t OpenDataSize;
};

struct OsNodeInfo {
    OsUtf8Char Name[OS_DEVICE_NAME_MAX];
};

extern OsStatus OsNodeOpen(struct OsNodeCreateInfo CreateInfo, OsNodeHandle *OutHandle);

extern OsStatus OsNodeClose(OsNodeHandle Handle);

extern OsStatus OsNodeQueryInterface(OsNodeHandle Handle, struct OsNodeQueryInterfaceInfo Query, OsDeviceHandle *OutHandle);

extern OsStatus OsNodeStat(OsNodeHandle Handle, struct OsNodeInfo *OutInfo);

/// @} // group OsNode


/// @defgroup OsFolderIterator Folder Iterator
/// @{

typedef uint16_t OsFolderEntryType;
typedef uint16_t OsFolderEntryNameSize;

struct OsFolderIterateCreateInfo {
    struct OsPath Path;
};

#if defined(__clang__) && defined(__cplusplus)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wc99-extensions"
#endif

struct OsFolderEntry {
    OsFolderEntryNameSize NameSize;

    OsUtf8Char Name[] OS_COUNTED_BY(NameSize);
};

#if defined(__clang__) && defined(__cplusplus)
#   pragma clang diagnostic pop
#endif

#define OS_FOLDER_ITERATOR_INVALID ((OsFolderIteratorHandle)(0))

/// @brief Create a directory iterator.
///
/// @pre @p CreateInfo.PathFront and @p CreateInfo.PathBack must point to the front and back of a valid buffer.
/// @pre @p OutHandle must point to valid memory.
///
/// @post @p OutHandle will be a valid folder iterator handle if the operation is successful.
/// @post @p OutHandle will be @a OsFolderIteratorHandleInvalid if the operation fails.
///
/// @param OsFolderIterateCreateInfo The create information for the folder iterator.
/// @param OutHandle The folder iterator handle.
///
/// @return The status of the operation.
extern OsStatus OsFolderIterateCreate(struct OsFolderIterateCreateInfo CreateInfo, OsFolderIteratorHandle *OutHandle);

/// @brief End the directory iteration.
///
/// @pre @p Handle must be a valid folder iterator handle.
///
/// @param Handle The folder iterator handle.
///
/// @return The status of the operation.
extern OsStatus OsFolderIterateDestroy(OsFolderIteratorHandle Handle);

/// @brief Iterate to the next entry in the directory.
///
/// @pre @p Handle must be a valid folder iterator handle.
/// @pre @p OutEntry must point to valid memory.
///
/// @post @p OutEntry will contain the next entry in the directory.
///
/// @param Handle The folder iterator handle.
/// @param OutEntry The next entry in the directory.
///
/// @return The status of the operation.
extern OsStatus OsFolderIterateNext(OsFolderIteratorHandle Handle, struct OsFolderEntry *OutEntry);

/// @} // group OsFolderIterator

#ifdef __cplusplus
}
#endif
