#pragma once

#include "shared/status.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_OBJECT_HANDLE(name) typedef struct name##Handle *name

#ifdef __cplusplus
#   define OS_STATIC_ASSERT(expr, message) static_assert(expr, message)
#else
#   define OS_STATIC_ASSERT(expr, message) _Static_assert(expr, message)
#endif

typedef uint8_t OsByte;
typedef char OsUtf8Char;

enum {
    eOsCallFileOpen = 0x10,
    eOsCallFileClose = 0x11,
    eOsCallFileRead = 0x12,
    eOsCallFileWrite = 0x13,
    eOsCallFileSeek = 0x14,
    eOsCallFileStat = 0x15,
    eOsCallFileControl = 0x16,

    eOsCallDirIter = 0x20,
    eOsCallDirNext = 0x21,

    eOsCallProcessGetCurrent = 0x30,
    eOsCallProcessCreate = 0x31,
    eOsCallProcessExit = 0x32,

    eOsCallThreadGetCurrent = 0x40,
    eOsCallThreadCreate = 0x41,
    eOsCallThreadControl = 0x42,
    eOsCallThreadDestroy = 0x43,

    eOsCallTransactionBegin = 0x50,
    eOsCallTransactionCommit = 0x51,
    eOsCallTransactionRollback = 0x52,

    eOsCallQueryParam = 0x60,
    eOsCallUpdateParam = 0x61,

    eOsCallCount = 0x100,
};

/// @brief A handle to a file object.
OS_OBJECT_HANDLE(OsFileHandle);

/// @brief A handle to a folder iterator.
OS_OBJECT_HANDLE(OsFolderIteratorHandle);

/// @brief A handle to a transaction.
OS_OBJECT_HANDLE(OsTransactionHandle);

/// @brief A handle to a thread.
OS_OBJECT_HANDLE(OsThreadHandle);

/// @brief A handle to a process.
OS_OBJECT_HANDLE(OsProcessHandle);


/// @defgroup OsFile Files
/// @{

/// @brief An invalid file handle.
#define OS_FILE_INVALID ((OsFileHandle)(UINT64_MAX))

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

/// @brief Open or create a file.
///
/// @pre @p PathFront and @p PathBack must point to the front and back of a valid buffer.
/// @pre @p PathBack must be > @p PathFront.
/// @pre @p OutHandle must point to valid memory.
/// @pre @p Mode must be a valid combination of @a OsFileOpenMode flags.
///
/// @post @p OutHandle will be a valid file handle if the operation is successful.
/// @post @p OutHandle will be @a OsFileHandleInvalid if the operation fails.
///
/// @param PathFront The front of the path.
/// @param PathBack The back of the path.
/// @param Mode The mode to open the file in.
/// @param OutHandle The file handle.
///
/// @return The status of the operation.
extern OsStatus OsFileOpen(const char *PathFront, const char *PathBack, OsFileOpenMode Mode, OsFileHandle *OutHandle);

extern OsStatus OsFileClose(OsFileHandle Handle);

extern OsStatus OsFileRead(OsFileHandle Handle, void *BufferFront, void *BufferBack, uint64_t *OutRead);

extern OsStatus OsFileWrite(OsFileHandle Handle, const void *BufferFront, const void *BufferBack, uint64_t *OutWritten);

extern OsStatus OsFileSeek(OsFileHandle Handle, OsSeekMode Mode, int64_t Offset, uint64_t *OutPosition);

extern OsStatus OsFileControl(OsFileHandle, uint64_t ControlCode, void *ControlData, size_t ControlSize);

/// @} // group OsFile




/// @defgroup OsFolderIterator Folder Iterator
/// @{

typedef uint16_t OsFolderEntryType;
typedef uint16_t OsFolderEntryNameSize;

struct OsFolderEntry {
    OsFolderEntryType Type;
    OsFolderEntryNameSize NameSize;

    OsUtf8Char Name[];
};

#define OS_FOLDER_ITERATOR_INVALID ((OsFolderIteratorHandle)(0))

/// @brief Create a directory iterator.
///
/// @pre @p PathFront and @p PathBack must point to the front and back of a valid buffer.
/// @pre @p PathBack must be > @p PathFront.
/// @pre @p OutHandle must point to valid memory.
///
/// @post @p OutHandle will be a valid folder iterator handle if the operation is successful.
/// @post @p OutHandle will be @a OsFolderIteratorHandleInvalid if the operation fails.
///
/// @param PathFront The front of the path.
/// @param PathBack The back of the path.
/// @param OutHandle The folder iterator handle.
///
/// @return The status of the operation.
extern OsStatus OsFolderIterate(const char *PathFront, const char *PathBack, OsFolderIteratorHandle *OutHandle);

extern OsStatus OsFolderIterateNext(OsFolderIteratorHandle Handle, struct OsFolderEntry *OutEntry);

extern OsStatus OsFolderIterateEnd(OsFolderIteratorHandle Handle);

/// @} // group OsFolderIterator

/// @defgroup OsTransaction Transactions
/// @{

#define OS_TRANSACTION_INVALID ((OsTransactionHandle)(0))

/// @brief Transaction creation modes.
///
/// Bits [0:1] contain the read transaction mode.
/// Bits [2:3] contain the write transaction mode.
/// Bits [4:63] are reserved and must be zero.
typedef uint64_t OsTransactionMode;

enum {
    eOsIsolateReadSerializable = UINT64_C(0),
    eOsIsolateReadCommitted = UINT64_C(1),
    eOsIsolateReadUncommitted = UINT64_C(2),

    eOsIsolateWriteSerializable = UINT64_C(0) << 2,
    eOsIsolateWriteCommitted = UINT64_C(1) << 2,
    eOsIsolateWriteUncommitted = UINT64_C(2) << 2,
};

extern OsStatus OsTransactBegin(const char *NameFront, const char *NameBack, OsTransactionMode Mode, OsTransactionHandle *OutHandle);

extern OsStatus OsTransactCommit(OsTransactionHandle Handle);

extern OsStatus OsTransactRollback(OsTransactionHandle Handle);

/// @} // group OsTransaction




/// @defgroup OsParam Params
/// @{

/// @brief The type of the param.
typedef uint32_t OsParamType;

/// @brief Additional flags for the param.
/// Bits [0:7] contain the field type.
/// Bits [8:12] are reserved and must be zero.
/// Bit [13] is set if an error occurred while reading the param.
/// Bit [14] is set if the param is read only.
/// Bit [15] is set if the param is an array.
/// Bits [16:31] is the size of the array if the array flag is set. Otherwise it is reserved.
typedef uint32_t OsParamFlags;

/// @brief The value of the param.
typedef uint64_t OsParamValue;

enum {
    eOsParamFlagVoid     = UINT32_C(0),
    eOsParamFlagBoolean  = UINT32_C(1),
    eOsParamFlagSigned   = UINT32_C(2),
    eOsParamFlagUnsigned = UINT32_C(3),
    eOsParamFlagFloat    = UINT32_C(4),
    eOsParamFlagString   = UINT32_C(5),

    eOsParamFlagError    = UINT32_C(1) << 13,
    eOsParamFlagConst    = UINT32_C(1) << 14,
    eOsParamFlagArray    = UINT32_C(1) << 15,
};

enum {
    eOsParamPowerState = UINT32_C(0),
    eOsParamPhysicalMemory = UINT32_C(1),
    eOsParamMaxNameLength = UINT32_C(2),
    eOsParamMaxPathLength = UINT32_C(3),
};

struct OsParamConfig {
    OsParamType Type;
    OsParamFlags Flags;
    OsParamValue Value;
};

/// @brief Query params for the current process and system.
///
/// @pre @p ConfigFront and @p ConfigBack must point to the front and back of a valid buffer.
/// @pre @p ConfigBack must be > @p ConfigFront.
/// @pre @p OutCount must point to valid memory.
///
/// @post @p OutCount will contain the number of params returned.
///
/// @param ConfigFront The front of the configuration buffer.
/// @param ConfigBack The back of the configuration buffer.
///
/// @return The status of the operation.
extern OsStatus OsParamSelect(struct OsParamConfig *ConfigFront, struct OsParamConfig *ConfigBack);

extern OsStatus OsParamUpdate(struct OsParamConfig *ConfigFront, struct OsParamConfig *ConfigBack);

/// @} // group OsParam

#ifdef __cplusplus
}
#endif
