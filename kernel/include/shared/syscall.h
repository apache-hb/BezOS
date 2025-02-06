#pragma once

#include "shared/status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    eOsCallFileOpen = 0x10,
    eOsCallFileClose = 0x11,
    eOsCallFileRead = 0x12,
    eOsCallFileWrite = 0x13,
    eOsCallFileSeek = 0x14,
    eOsCallFileStat = 0x15,

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
};

/// @brief A handle to a file object.
typedef uint64_t OsFileHandle;

enum {
    /// @brief An invalid file handle.
    OsFileHandleInvalid = UINT64_MAX
};

/// @brief Seek modes.
///
/// Bits [0:3] contain the seek mode.
/// Bits [4:63] are reserved and must be zero.
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
    eOsSeekAbsolute = 0,
    eOsSeekRelative = 1,
    eOsSeekEnd = 2,
};

/// @brief Open or create a file.
///
/// @pre @p PathFront and @p PathBack must be valid.
/// @pre @p PathBack must be > @p PathFront.
/// @pre @p OutHandle must be valid.
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

/// @brief A handle to a transaction.
typedef uint64_t OsTransactionHandle;

enum {
    /// @brief An invalid transaction handle.
    OsTransactionInvalid = UINT64_MAX
};

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

#ifdef __cplusplus
}
#endif
